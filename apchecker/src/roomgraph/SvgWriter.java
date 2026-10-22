package roomgraph;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Draws the plan as a justified graph: the outdoors at the top, and every room
 * placed on the row given by its connection-count distance from the outdoors. This is
 * the standard space-syntax drawing (Hillier and Hanson), and it is the drawing
 * in which the intimacy gradient is visible at a glance: a plan that obeys
 * pattern 127 has privacy increasing monotonically down the page.
 *
 * Rooms and connections named in a violation are outlined in the color of the worst
 * severity against them. The findings are listed underneath.
 *
 * Output is a self-contained SVG string: no external stylesheet, no fonts
 * beyond the generic families, so it opens in any browser or vector editor.
 */
public final class SvgWriter {

    private static final int MARGIN = 30;
    private static final int BOX_WIDTH = 150;
    private static final int BOX_HEIGHT = 44;
    private static final int COLUMN_GAP = 26;
    private static final int ROW_HEIGHT = 110;
    private static final int LINE_HEIGHT = 16;
    private static final int WRAP_COLUMNS = 110;

    private SvgWriter() {
        // Utility class.
    }

    /** The whole drawing, as a function of the plan and its findings. */
    public static String toSvg(PlanGraph plan, List<Violation> violations) {
        Map<Integer, List<String>> rows = layout(plan);
        Map<String, Point> centers = placeRooms(rows);
        Map<String, Severity> roomSeverity = worstSeverityByRoom(violations);
        Set<String> flaggedDoors = flaggedDoorKeys(violations);

        List<String> captionLines = captionLines(plan, violations);

        int width = drawingWidth(rows);
        int lastRowTop = MARGIN + (rows.size() - 1) * ROW_HEIGHT;
        int graphHeight = lastRowTop + BOX_HEIGHT + 2 * MARGIN;
        int height = graphHeight + LINE_HEIGHT * (captionLines.size() + 1) + MARGIN;

        StringBuilder svg = new StringBuilder();
        svg.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        svg.append("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"").append(width)
           .append("\" height=\"").append(height)
           .append("\" viewBox=\"0 0 ").append(width).append(' ').append(height).append("\">\n");
        svg.append("  <rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>\n");
        svg.append("  <style>\n")
           .append("    .label { font-family: Helvetica, Arial, sans-serif; font-size: 12px; }\n")
           .append("    .sub   { font-family: Helvetica, Arial, sans-serif; font-size: 9px;  fill: #555555; }\n")
           .append("    .note  { font-family: Helvetica, Arial, sans-serif; font-size: 11px; }\n")
           .append("  </style>\n");

        appendEdges(svg, plan, centers, flaggedDoors);
        appendRooms(svg, plan, centers, roomSeverity);
        appendCaption(svg, captionLines, graphHeight);

        svg.append("</svg>\n");
        return svg.toString();
    }

    /* ---------------------------------------------------------------- layout */

    /** Rooms grouped by their depth from the outdoors; unreachable rooms last. */
    private static Map<Integer, List<String>> layout(PlanGraph plan) {
        // The outdoors may be several vertices, or none. Multi-source: a room's
        // depth is its distance from the nearest outdoors.
        Map<String, Integer> depths = Graphs.depths(plan, plan.exteriorIds(), Set.of());
        Map<Integer, List<String>> rows = new LinkedHashMap<>();

        int deepest = 0;
        for (int depth : depths.values()) {
            deepest = Math.max(deepest, depth);
        }
        for (int depth = 0; depth <= deepest; depth++) {
            rows.put(depth, new ArrayList<>());
        }
        int orphanRow = deepest + 1;

        for (String id : plan.roomIds()) {
            Integer depth = depths.get(id);
            if (depth == null) {
                // A room with no path to the outdoors. Give it its own row
                // rather than dropping it: the fault must be visible.
                rows.computeIfAbsent(orphanRow, key -> new ArrayList<>()).add(id);
            } else {
                rows.get(depth).add(id);
            }
        }
        rows.values().removeIf(List::isEmpty);
        return rows;
    }

    private static Map<String, Point> placeRooms(Map<Integer, List<String>> rows) {
        int width = drawingWidth(rows);
        Map<String, Point> centers = new LinkedHashMap<>();
        int rowIndex = 0;
        for (List<String> row : rows.values()) {
            int rowWidth = row.size() * BOX_WIDTH + (row.size() - 1) * COLUMN_GAP;
            int left = (width - rowWidth) / 2;
            for (int i = 0; i < row.size(); i++) {
                int cx = left + i * (BOX_WIDTH + COLUMN_GAP) + BOX_WIDTH / 2;
                int cy = MARGIN + rowIndex * ROW_HEIGHT + BOX_HEIGHT / 2;
                centers.put(row.get(i), new Point(cx, cy));
            }
            rowIndex++;
        }
        return centers;
    }

    private static int drawingWidth(Map<Integer, List<String>> rows) {
        int widest = 1;
        for (List<String> row : rows.values()) {
            widest = Math.max(widest, row.size());
        }
        return 2 * MARGIN + widest * BOX_WIDTH + (widest - 1) * COLUMN_GAP;
    }

    /* ---------------------------------------------------------------- drawing */

    private static void appendEdges(StringBuilder svg, PlanGraph plan, Map<String, Point> centers, Set<String> flagged) {
        svg.append("  <g id=\"connections\">\n");
        for (Connection connection : plan.connections()) {
            Point a = centers.get(connection.a());
            Point b = centers.get(connection.b());
            boolean bad = flagged.contains(connection.key());
            boolean opening = connection.kind() == ConnectionKind.OPENING;

            // A door is drawn solid, an opening pale and thin: the eye should be
            // able to find the closable boundaries of the house at a glance, which
            // is most of what the privacy rules are about.
            String stroke = bad ? "#c0392b" : (opening ? "#ccd1d3" : "#7f8c8d");
            String dash = bad ? " stroke-dasharray=\"6 4\"" : "";
            int strokeWidth = bad ? 3 : (opening ? 1 : 2);
            svg.append("    <line x1=\"").append(a.x()).append("\" y1=\"").append(a.y())
               .append("\" x2=\"").append(b.x()).append("\" y2=\"").append(b.y())
               .append("\" stroke=\"").append(stroke)
               .append("\" stroke-width=\"").append(strokeWidth).append('"').append(dash).append("/>\n");
        }
        svg.append("  </g>\n");
    }

    private static void appendRooms(StringBuilder svg, PlanGraph plan, Map<String, Point> centers, Map<String, Severity> severities) {
        svg.append("  <g id=\"rooms\">\n");
        for (Room room : plan.rooms()) {
            Point center = centers.get(room.id());
            int x = center.x() - BOX_WIDTH / 2;
            int y = center.y() - BOX_HEIGHT / 2;
            Severity severity = severities.get(room.id());
            String stroke = (severity == null) ? "#2c3e50" : severityColor(severity);
            int strokeWidth = (severity == null) ? 1 : 3;

            svg.append("    <rect x=\"").append(x).append("\" y=\"").append(y)
               .append("\" width=\"").append(BOX_WIDTH).append("\" height=\"").append(BOX_HEIGHT)
               .append("\" rx=\"5\" fill=\"").append(privacyFill(room.privacy()))
               .append("\" stroke=\"").append(stroke)
               .append("\" stroke-width=\"").append(strokeWidth).append("\"/>\n");

            svg.append("    <text class=\"label\" x=\"").append(center.x())
               .append("\" y=\"").append(center.y() - 1)
               .append("\" text-anchor=\"middle\">").append(escape(room.label())).append("</text>\n");

            svg.append("    <text class=\"sub\" x=\"").append(center.x())
               .append("\" y=\"").append(center.y() + 13)
               .append("\" text-anchor=\"middle\">")
               .append(escape(room.kind() + " / " + room.privacy()))
               .append("</text>\n");
        }
        svg.append("  </g>\n");
    }

    private static void appendCaption(StringBuilder svg, List<String> lines, int top) {
        svg.append("  <g id=\"findings\">\n");
        int y = top;
        for (String line : lines) {
            String color = "#2c3e50";
            if (line.startsWith("[CODE]")) {
                color = severityColor(Severity.CODE);
            } else if (line.startsWith("[PATTERN]")) {
                color = severityColor(Severity.PATTERN);
            } else if (line.startsWith("[ADVICE]")) {
                color = severityColor(Severity.ADVICE);
            }
            svg.append("    <text class=\"note\" x=\"").append(MARGIN).append("\" y=\"").append(y)
               .append("\" fill=\"").append(color).append("\">").append(escape(line)).append("</text>\n");
            y += LINE_HEIGHT;
        }
        svg.append("  </g>\n");
    }

    /* ---------------------------------------------------------------- helpers */

    private static Map<String, Severity> worstSeverityByRoom(List<Violation> violations) {
        Map<String, Severity> worst = new LinkedHashMap<>();
        for (Violation violation : violations) {
            for (String id : violation.rooms()) {
                Severity current = worst.get(id);
                // Severity is declared worst-first, so the lower ordinal wins.
                if (current == null || violation.severity().ordinal() < current.ordinal()) {
                    worst.put(id, violation.severity());
                }
            }
        }
        return worst;
    }

    private static Set<String> flaggedDoorKeys(List<Violation> violations) {
        Set<String> keys = new LinkedHashSet<>();
        for (Violation violation : violations) {
            for (Connection connection : violation.connections()) {
                keys.add(connection.key());
            }
        }
        return keys;
    }

    private static List<String> captionLines(PlanGraph plan, List<Violation> violations) {
        List<String> lines = new ArrayList<>();
        lines.add(plan.name() + " \u2014 justified graph, rooted at the outdoors. "
                + violations.size() + " finding(s).");
        if (violations.isEmpty()) {
            lines.add("No findings.");
            return lines;
        }
        for (Violation violation : violations) {
            String text = "[" + violation.severity() + "] " + violation.ruleId() + ": " + violation.message();
            lines.addAll(wrap(text, WRAP_COLUMNS));
        }
        return lines;
    }

    /** Naive word wrap; long messages must not run off the edge of the drawing. */
    private static List<String> wrap(String text, int columns) {
        List<String> lines = new ArrayList<>();
        StringBuilder current = new StringBuilder();
        for (String word : text.split(" ")) {
            if (current.length() > 0 && current.length() + 1 + word.length() > columns) {
                lines.add(current.toString());
                current = new StringBuilder("    ");
            }
            if (current.length() > 0 && !current.toString().endsWith("    ")) {
                current.append(' ');
            }
            current.append(word);
        }
        if (current.length() > 0) {
            lines.add(current.toString());
        }
        return lines;
    }

    private static String privacyFill(PrivacyLevel level) {
        switch (level) {
            case EXTERIOR:
                return "#ecf0f1";
            case ENTRY:
                return "#e8f6f3";
            case PUBLIC:
                return "#d6eaf8";
            case SEMI_PRIVATE:
                return "#d4e6f1";
            case PRIVATE:
                return "#e8daef";
            default:
                throw new IllegalArgumentException("Unhandled privacy level: " + level);
        }
    }

    private static String severityColor(Severity severity) {
        switch (severity) {
            case CODE:
                return "#c0392b";
            case PATTERN:
                return "#d68910";
            case ADVICE:
                return "#7f8c8d";
            default:
                throw new IllegalArgumentException("Unhandled severity: " + severity);
        }
    }

    private static String escape(String raw) {
        return raw.replace("&", "&amp;")
                  .replace("<", "&lt;")
                  .replace(">", "&gt;")
                  .replace("\"", "&quot;")
                  .replace("'", "&apos;");
    }

    /** A point in the drawing. */
    private record Point(int x, int y) {
    }
}
