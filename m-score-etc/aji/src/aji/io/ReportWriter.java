package aji.io;

import aji.diagnose.Diagnosis;
import aji.model.Dep;
import aji.score.Module;
import aji.score.ModuleScore;
import aji.score.Params;
import aji.score.Result;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.stream.Collectors;

/**
 * Formats a {@link Result} as a plain-text report and as a CSV table of modules.
 * Tables show three decimals; the headline numbers show six, for exact comparison.
 */
public final class ReportWriter {

    private ReportWriter() {
    }

    /**
     * Everything the report needs.
     *
     * @param main        the result under the chosen parameters
     * @param variants    the same system under the four combinations of the open choices
     * @param source      the input file, as given
     * @param language    "Java", "C++" or null for a hand-made model
     * @param layering    "declared", "derived" or "given"
     * @param dropped     files left out because no declared layer matched them
     * @param notes       remarks from dependency extraction
     * @param derivedOnly the same files with derived layers, or null
     * @param top         how many modules to list
     * @param explained   what joins the modules that lose the most; may be empty
     */
    public record Input(Result main, List<Result> variants, String source, String language,
                        String layering, List<String> dropped, List<String> notes,
                        Result derivedOnly, int top, List<Diagnosis> explained) {
    }

    public static String text(Input in) {
        Result r = in.main();
        StringBuilder b = new StringBuilder();
        b.append("aji: modularity report for ").append(r.model().name()).append('\n');
        b.append("Input: ").append(in.source()).append("\n\n");

        b.append("INPUT\n");
        if (in.language() != null) {
            b.append("  Language:      ").append(in.language()).append('\n');
        }
        b.append("  Files:         ").append(r.model().files().size()).append('\n');
        if (!in.dropped().isEmpty()) {
            b.append("  Left out:      ").append(in.dropped().size())
                    .append(" files that match no declared layer").append('\n');
        }
        b.append("  Dependencies:  ").append(r.model().deps().size())
                .append(" distinct ordered file pairs\n");
        b.append("  Layers:        ").append(r.layerCount()).append(", ").append(in.layering())
                .append('\n');
        for (String note : in.notes()) {
            b.append("  Note: ").append(note).append('\n');
        }
        b.append('\n');

        b.append("RESULT\n");
        b.append("  M-score          = ").append(pct6(r.mScore())).append(" %   (")
                .append(r.params().label()).append(")\n");
        b.append("  Propagation cost = ").append(pct6(r.propagationCost())).append(" %\n\n");
        b.append("  The published definition leaves two choices open. The score under each:\n");
        b.append("                  lower=ALL  lower=ADJACENT\n");
        for (Params.CrossCount c : Params.CrossCount.values()) {
            b.append(String.format(Locale.ROOT, "    cross=%-6s", c));
            for (Params.LowerScope l : Params.LowerScope.values()) {
                double v = in.variants().stream()
                        .filter(x -> x.params().cross() == c && x.params().lower() == l)
                        .mapToDouble(Result::mScore).findFirst().orElse(Double.NaN);
                b.append(String.format(Locale.ROOT, "%11s", pct3(v)));
            }
            b.append('\n');
        }
        if (in.derivedOnly() != null) {
            b.append("\n  Same files with layers derived from the dependencies instead: ")
                    .append(pct3(in.derivedOnly().mScore())).append(" % (")
                    .append(in.derivedOnly().layerCount()).append(" layers, ")
                    .append(in.derivedOnly().modules().size()).append(" modules).\n")
                    .append("  That reading is lenient: every dependency then crosses layers, so the\n")
                    .append("  only modules larger than one file are dependency cycles.\n");
        }
        b.append('\n');

        b.append("WHERE THE SCORE IS LOST (percentage points; the three add up to 100 - M-score)\n");
        b.append(String.format(Locale.ROOT, "  %-44s%9s%n", "Module size (SP)", pct3(r.sizeLoss())));
        b.append(String.format(Locale.ROOT, "  %-44s%9s%n", "Dependencies from lower layers (CLDDP)",
                pct3(r.crossLoss())));
        b.append(String.format(Locale.ROOT, "  %-44s%9s%n", "Dependencies inside modules (IMCP)",
                pct3(r.innerLoss())));
        b.append('\n');

        b.append("LAYERS (1 is the top, the most depended upon)\n");
        b.append(String.format(Locale.ROOT, "  %5s %6s  %-24s %6s %8s %8s%n",
                "rank", "given", "name", "files", "modules", "largest"));
        Map<Integer, List<ModuleScore>> byRank = new HashMap<>();
        r.modules().forEach(ms -> byRank.computeIfAbsent(ms.module().layer(), k -> new ArrayList<>())
                .add(ms));
        for (int rank = 1; rank <= r.layerCount(); rank++) {
            List<ModuleScore> ms = byRank.getOrDefault(rank, List.of());
            int given = r.declaredLayers().get(rank - 1);
            int files = ms.stream().mapToInt(x -> x.module().size()).sum();
            int largest = ms.stream().mapToInt(x -> x.module().size()).max().orElse(0);
            String name = r.model().layerNames().getOrDefault(given, "");
            b.append(String.format(Locale.ROOT, "  %5d %6d  %-24s %6d %8d %8d%n",
                    rank, given, clip(name, 24), files, ms.size(), largest));
        }
        b.append('\n');

        List<ModuleScore> ranked = new ArrayList<>(r.modules());
        ranked.sort(ModuleScore.BY_LOSS);
        int shown = Math.min(in.top(), ranked.size());
        b.append("MODULES THAT LOSE THE MOST (").append(shown).append(" of ")
                .append(ranked.size()).append("; lost and contrib in percentage points)\n");
        b.append(String.format(Locale.ROOT, "  %8s %5s %6s %6s %6s %6s %8s  %s%n",
                "lost", "layer", "files", "SP", "CLDDP", "IMCP", "contrib", "module"));
        for (int k = 0; k < shown; k++) {
            ModuleScore ms = ranked.get(k);
            Module m = ms.module();
            b.append(String.format(Locale.ROOT, "  %8s %5d %6d %6.3f %6.3f %6.3f %8s  %s%n",
                    pct3(ms.lost()), m.layer(), m.size(), ms.sp(), ms.clddp(), ms.imcp(),
                    pct3(ms.contribution()), m.label()));
        }
        b.append('\n');

        if (!in.explained().isEmpty()) {
            b.append(explanation(in.explained())).append('\n');
        }

        List<Dep> up = r.edges().upward();
        b.append("LAYERING VIOLATIONS (upward dependencies, left out of the score): ")
                .append(up.size()).append('\n');
        Map<String, Integer> rankOf = new HashMap<>();
        r.modules().forEach(ms -> ms.module().files().forEach(f -> rankOf.put(f, ms.module().layer())));
        int limit = Math.min(up.size(), 100);
        for (int k = 0; k < limit; k++) {
            Dep d = up.get(k);
            b.append("  ").append(d.from()).append(" -> ").append(d.to()).append("   (layer ")
                    .append(rankOf.get(d.from())).append(" -> ").append(rankOf.get(d.to()))
                    .append(")\n");
        }
        if (up.size() > limit) {
            b.append("  ... ").append(up.size() - limit).append(" more\n");
        }
        if (!in.dropped().isEmpty()) {
            b.append("\nFILES LEFT OUT (no declared layer matched): ").append(in.dropped().size())
                    .append('\n');
            in.dropped().stream().limit(40).forEach(f -> b.append("  ").append(f).append('\n'));
            if (in.dropped().size() > 40) {
                b.append("  ... ").append(in.dropped().size() - 40).append(" more\n");
            }
        }
        return b.toString();
    }

    /**
     * The section of the report that explains why the modules that lose the most lose their
     * points: what joins a large module and what the score would be with a part of it as a
     * layer of its own, the dependency cycles inside it, and the dependencies on it from
     * lower layers.
     */
    public static String explanation(List<Diagnosis> explained) {
        StringBuilder b = new StringBuilder();
        b.append("WHY THE COSTLIEST MODULES LOSE POINTS (").append(explained.size())
                .append(" explained)\n");
        b.append("  \"Pieces\" are what is left of a module without one folder or one file:"
                + " groups of files\n  that are no longer linked to one another.\n");
        for (Diagnosis d : explained) {
            b.append('\n');
            ModuleScore ms = d.module();
            Module m = ms.module();
            b.append("  ").append(m.label()).append(", layer ").append(m.layer())
                    .append(": loses ").append(pct3(ms.lost())).append(" points (")
                    .append(pct3(ms.sizeLoss())).append(" to size, ")
                    .append(pct3(ms.crossLoss())).append(" to dependencies from lower layers, ")
                    .append(pct3(ms.innerLoss())).append(" to dependencies inside)\n");
            if (d.sized()) {
                explainSize(b, d);
            }
            explainCycles(b, d);
            explainCrossings(b, d);
        }
        return b.toString();
    }

    /** What joins a module of five or more files, and what could divide it. */
    private static void explainSize(StringBuilder b, Diagnosis d) {
        Module m = d.module().module();
        String dir = m.directory().isEmpty() ? "the project root" : m.directory();
        if (d.folders().isEmpty()) {
            b.append("    Folders: none. All its files are directly in ").append(dir).append(".\n");
        } else {
            b.append("    Folders directly below ").append(dir).append(" (\"uses\" and \"used by\""
                    + " count dependencies on and from the module's other folders)\n");
            b.append(String.format(Locale.ROOT, "    %7s %6s %8s  %-28s %s%n", "files", "uses",
                    "used by", "folder", "the rest of the module without it"));
            for (Diagnosis.Part p : first(d.folders(), 12)) {
                b.append(String.format(Locale.ROOT, "    %7d %6d %8d  %-28s %s%n",
                        p.files().size(), p.uses(), p.usedBy(), clip(p.name(), 28), pieces(p)));
            }
            more(b, d.folders().size(), 12);
            b.append("    Dependencies between the folders\n");
            for (Diagnosis.Link l : first(d.links(), 8)) {
                b.append(String.format(Locale.ROOT, "    %7d  %s -> %s%n", l.deps(), l.from(),
                        l.to()));
            }
            more(b, d.links().size(), 8);
        }

        if (d.joiningFiles().isEmpty()) {
            b.append("    Joining files: none. Without any one file, the rest of the module"
                    + " stays in one piece.\n");
        } else {
            b.append("    Joining files (").append(d.joiningFiles().size())
                    .append("; those that cut off the most files first)\n");
            for (Diagnosis.Part p : first(d.joiningFiles(), 5)) {
                b.append(String.format(Locale.ROOT, "      %-48s %s%n", clip(p.name(), 48),
                        pieces(p)));
            }
            more(b, d.joiningFiles().size(), 5);
        }

        if (d.whatIfs().isEmpty()) {
            b.append("    What if: nothing to offer. No folder, and none of the joining files"
                    + " tried, can become a layer\n    of its own with a gain and without a new"
                    + " layering violation.\n");
        } else {
            b.append("    What if (one change at a time; none adds a layering violation)\n");
            for (Diagnosis.WhatIf w : first(d.whatIfs(), 5)) {
                Diagnosis.Part p = w.part();
                String what = !p.name().endsWith("/") && p.files().size() == 1 ? p.name()
                        : p.name() + " (" + p.files().size()
                        + (p.files().size() == 1 ? " file)" : " files)");
                b.append(String.format(Locale.ROOT, "      %+7.3f  %s as a layer of its own just"
                                + " %s layer %d: M-score %s %%, %d modules in place of 1,"
                                + " the largest of %d files%n",
                        100 * w.gain(), what, w.above() ? "above" : "below", m.layer(),
                        pct3(w.mScore()), w.modules(), w.largest()));
            }
            more(b, d.whatIfs().size(), 5);
        }
    }

    /** The dependency cycles of a module, and how far layers inside it could divide it. */
    private static void explainCycles(StringBuilder b, Diagnosis d) {
        if (d.module().module().size() < 2) {
            return;
        }
        if (d.cycles().isEmpty()) {
            b.append("    Cycles: none.\n");
        } else {
            int held = 0;
            for (Diagnosis.Cycle c : d.cycles()) {
                held += c.files().size();
            }
            b.append("    Cycles (").append(d.cycles().size()).append(", holding ").append(held)
                    .append(" of its files; the files of a cycle cannot be put in different"
                            + " layers)\n");
            for (Diagnosis.Cycle c : first(d.cycles(), 5)) {
                b.append("      ").append(c.files().size()).append(" files, ").append(c.deps())
                        .append(" dependencies: ").append(listed(c.files(), 8)).append('\n');
                if (!c.mutual().isEmpty()) {
                    List<String> pairs = c.mutual().stream()
                            .map(p -> p.a() + " <-> " + p.b()).toList();
                    b.append("        depend on each other directly: ").append(listed(pairs, 6))
                            .append('\n');
                } else if (!c.example().isEmpty()) {
                    b.append("        for example ").append(String.join(" -> ", c.example()))
                            .append(" -> ").append(c.example().get(0)).append('\n');
                }
            }
            more(b, d.cycles().size(), 5);
        }
        Diagnosis.Finest f = d.finest();
        if (f != null && f.modules() == 1) {
            b.append("    Layers inside it cannot divide it: all its files are in one cycle.\n");
        } else if (f != null) {
            String sizes = f.largest() == 1 ? " of one file each"
                    : ", the largest of " + f.largest() + " files";
            b.append(String.format(Locale.ROOT, "    Layered as finely as its cycles allow"
                            + " (%d layers), it would be %d modules%s: M-score %s %% (%+.3f)\n",
                    f.layers(), f.modules(), sizes, pct3(f.mScore()), 100 * f.gain()));
        }
    }

    /** The dependencies on a module from lower layers: how many, on which files, from which. */
    private static void explainCrossings(StringBuilder b, Diagnosis d) {
        ModuleScore ms = d.module();
        Diagnosis.Crossings x = d.crossings();
        if (ms.lowerFiles() == 0) {
            b.append("    From lower layers: nothing. It is in the bottom layer.\n");
            return;
        }
        String below = x.scope() == Params.LowerScope.ALL ? "the layers below"
                : "the next layer down";
        if (x.pairs() == 0) {
            b.append("    From lower layers: no dependencies from the ").append(ms.lowerFiles())
                    .append(" files of ").append(below).append(".\n");
            return;
        }
        int size = ms.module().size();
        b.append(String.format(Locale.ROOT, "    From lower layers: %d dependencies, from %d of"
                + " the %d files of %s", x.pairs(), x.sources().size(), ms.lowerFiles(), below));
        if (size > 1) {
            b.append(String.format(Locale.ROOT, ", on %d of its %d files", x.targets().size(),
                    size));
        }
        b.append('\n');
        b.append(String.format(Locale.ROOT, "      The score counts %d %s against %d possible"
                        + " (CLDDP %.3f). Each one costs %.4f points.\n",
                ms.lowerDeps(), x.counted() == Params.CrossCount.PAIRS ? "dependencies"
                        : "lower files", (long) ms.lowerFiles() * size, ms.clddp(),
                100 * ms.costPerLowerDep()));
        if (size > 1) {
            b.append("      Its files most depended upon from below\n");
            counts(b, x.targets());
        }
        b.append("      The lower files that depend on ")
                .append(size > 1 ? "the most of its files\n" : "it\n");
        counts(b, x.sources());
    }

    private static void counts(StringBuilder b, List<Diagnosis.Count> counts) {
        for (Diagnosis.Count c : first(counts, 5)) {
            b.append(String.format(Locale.ROOT, "      %7d  %s\n", c.deps(), c.name()));
        }
        more(b, counts.size(), 5);
    }

    /** The first items of a list, separated by commas, and how many are left out. */
    private static String listed(List<String> items, int max) {
        return String.join(", ", first(items, max))
                + (items.size() > max ? ", ... (" + (items.size() - max) + " more)" : "");
    }

    /** "1 piece", or the number of pieces and the sizes of the ten largest. */
    private static String pieces(Diagnosis.Part p) {
        List<Integer> sizes = p.pieces();
        if (sizes.size() == 1) {
            return "1 piece";
        }
        return sizes.size() + " pieces: " + first(sizes, 10).stream().map(String::valueOf)
                .collect(Collectors.joining(", ")) + (sizes.size() > 10 ? ", ..." : "");
    }

    private static <T> List<T> first(List<T> list, int k) {
        return list.subList(0, Math.min(k, list.size()));
    }

    private static void more(StringBuilder b, int size, int shown) {
        if (size > shown) {
            b.append("      ... ").append(size - shown).append(" more\n");
        }
    }

    /** One row per module, full precision, for spreadsheets and trend tracking. */
    public static String modulesCsv(Result r) {
        StringBuilder b = new StringBuilder();
        b.append("layer_rank,layer_given,files,lower_deps,lower_files,internal_deps,"
                + "sp,clddp,imcp,weight,contribution,lost,size_loss,cross_loss,inner_loss,module,first_file\n");
        for (ModuleScore ms : r.modules()) {
            Module m = ms.module();
            b.append(m.layer()).append(',').append(r.declaredLayers().get(m.layer() - 1)).append(',')
                    .append(m.size()).append(',').append(ms.lowerDeps()).append(',')
                    .append(ms.lowerFiles()).append(',').append(ms.internalDeps()).append(',')
                    .append(num(ms.sp())).append(',').append(num(ms.clddp())).append(',')
                    .append(num(ms.imcp())).append(',').append(num(ms.weight())).append(',')
                    .append(num(ms.contribution())).append(',').append(num(ms.lost())).append(',')
                    .append(num(ms.sizeLoss())).append(',').append(num(ms.crossLoss())).append(',')
                    .append(num(ms.innerLoss())).append(',').append(csv(m.label())).append(',')
                    .append(csv(m.files().get(0))).append('\n');
        }
        return b.toString();
    }

    private static String pct3(double v) {
        return String.format(Locale.ROOT, "%.3f", 100 * v);
    }

    private static String pct6(double v) {
        return String.format(Locale.ROOT, "%.6f", 100 * v);
    }

    private static String num(double v) {
        return String.format(Locale.ROOT, "%.9f", v);
    }

    private static String csv(String s) {
        return "\"" + s.replace("\"", "\"\"") + "\"";
    }

    private static String clip(String s, int max) {
        return s.length() <= max ? s : s.substring(0, max - 1) + "~";
    }
}
