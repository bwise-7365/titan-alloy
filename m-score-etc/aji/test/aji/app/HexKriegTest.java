package aji.app;

import aji.model.Dep;
import aji.model.SystemModel;
import aji.testing.Checks;

import java.util.List;
import java.util.Set;
import java.util.TreeSet;

/**
 * Runs aji on HexKrieg and checks the result against HexKrieg's architecture (its CLAUDE.md):
 * the headless engine in {@code src/} is six packages that form layers, a game such as
 * Panzergruppe Guderian in {@code src-pgg/} uses the engine and no other game, and the Swing
 * front end in {@code src-gui/} and the continuous-motion simulation in {@code src-hksim/}
 * sit on top of both. The front ends are four layers: the widgets of the Swing front end
 * ({@code src-gui/widgets/}), the vehicle of the simulation ({@code src-hksim/vehicle/}),
 * its tank ({@code src-hksim/tank/}), and the windows of both. Run it from the aji folder,
 * or pass the project file:
 *
 * <pre>  java -cp out aji.app.HexKriegTest [projects/hexkrieg.aji.xml]</pre>
 *
 * HexKrieg changes from day to day, so the test fixes no file count and no score. It prints
 * them and checks the rules that must hold whatever they are.
 */
public final class HexKriegTest {

    private HexKriegTest() {
    }

    public static void main(String[] args) {
        ProjectRun.test(args, "projects/hexkrieg.aji.xml", HexKriegTest::architecture);
    }

    static void architecture(ProjectRun run, Checks c) {
        SystemModel m = run.model();
        Set<Integer> declared = new TreeSet<>(m.layerNames().keySet());
        declared.removeIf(k -> m.layerNames().get(k).equals("tests"));
        c.check("every declared layer but the tests holds files",
                declared.equals(m.filesByLayer().keySet()),
                "declared " + declared + ", with files " + m.filesByLayer().keySet());

        List<String> tests = run.files(id -> id.startsWith("test/"));
        c.check("the tests are left out, as the project file says", tests.isEmpty(),
                tests.size() + " files under test/");

        List<Dep> upward = run.result().edges().upward();
        c.check("no layering violation: no package of the engine uses a later one, the engine"
                + " uses no game and no front end, and no layer of the front ends uses a"
                + " later one", upward.isEmpty(), ProjectRun.listing(upward));

        List<Dep> acrossGames = run.deps(d -> game(d.from()) && game(d.to())
                && !ProjectRun.part(d.from(), 0).equals(ProjectRun.part(d.to(), 0)));
        c.check("no game uses another game's files", acrossGames.isEmpty(),
                ProjectRun.listing(acrossGames));

        c.check("each game uses the engine",
                run.files(HexKriegTest::game).stream().map(id -> ProjectRun.part(id, 0)).distinct()
                        .allMatch(root -> !run.deps(d -> d.from().startsWith(root + "/")
                                && d.to().startsWith("src/")).isEmpty()),
                "a game's source root has no dependency on src/");
        c.check("the Swing front end uses the engine",
                !run.deps(d -> d.from().startsWith("src-gui/") && d.to().startsWith("src/"))
                        .isEmpty(), "no dependency from src-gui/ to src/ was found");
        c.check("the continuous-motion simulation uses the engine",
                !run.deps(d -> d.from().startsWith("src-hksim/") && d.to().startsWith("src/"))
                        .isEmpty(), "no dependency from src-hksim/ to src/ was found");

        List<Dep> pastWidgets = run.deps(d -> d.from().startsWith("src-hksim/")
                && d.to().startsWith("src-gui/") && !d.to().startsWith("src-gui/widgets/"));
        c.check("of the Swing front end, the simulation uses the widgets alone",
                pastWidgets.isEmpty(), ProjectRun.listing(pastWidgets));

        List<Dep> headless = run.deps(d -> d.to().startsWith("src-gui/")
                && (d.from().startsWith("src-hksim/vehicle/")
                || d.from().startsWith("src-hksim/tank/")));
        c.check("the vehicle and the tank of the simulation use nothing of the Swing front end",
                headless.isEmpty(), ProjectRun.listing(headless));
    }

    /** True for a file of a game: one in a source root src-NAME other than the two front ends. */
    private static boolean game(String id) {
        String root = ProjectRun.part(id, 0);
        return root.startsWith("src-") && !root.equals("src-gui") && !root.equals("src-hksim");
    }
}
