package aji.app;

import aji.model.Dep;
import aji.testing.Checks;

import java.util.List;
import java.util.Set;

/**
 * Runs aji on HexKrieg and checks the result against HexKrieg's architecture (its CLAUDE.md):
 * the headless engine in {@code src/} must compile alone, and the Swing front end in
 * {@code src-gui/} and the continuous-motion simulation in {@code src-hksim/} sit on top of
 * it. Run it from the aji folder, or pass the project file:
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
        Set<Integer> layers = run.model().filesByLayer().keySet();
        c.check("the engine layer and the front-end layer both hold files",
                layers.contains(1) && layers.contains(2), "layers with files: " + layers);

        List<String> tests = run.files(id -> id.startsWith("test/"));
        c.check("the tests are left out, as the project file says", tests.isEmpty(),
                tests.size() + " files under test/");

        List<Dep> upward = run.result().edges().upward();
        c.check("no layering violation: the engine in src/ uses neither front end",
                upward.isEmpty(), ProjectRun.listing(upward));

        c.check("the Swing front end uses the engine",
                !run.deps(d -> d.from().startsWith("src-gui/") && d.to().startsWith("src/"))
                        .isEmpty(), "no dependency from src-gui/ to src/ was found");
        c.check("the continuous-motion simulation uses the engine",
                !run.deps(d -> d.from().startsWith("src-hksim/") && d.to().startsWith("src/"))
                        .isEmpty(), "no dependency from src-hksim/ to src/ was found");
    }
}
