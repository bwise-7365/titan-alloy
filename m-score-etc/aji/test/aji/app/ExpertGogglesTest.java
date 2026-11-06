package aji.app;

import aji.model.Dep;
import aji.model.SystemModel;
import aji.selftest.Checks;

import java.util.List;

/**
 * Runs aji on expert-goggles and checks the result against the architecture that
 * expert-goggles documents: five layers whose dependencies always point toward the foundation
 * ({@code restructure/README.md}), libraries in the facilities layer that are independent of
 * one another, and applications that use the shared support but never another application
 * ({@code apps/README.md}). Run it from the aji folder, or pass the project file:
 *
 * <pre>  java -cp out aji.app.ExpertGogglesTest [projects/expert-goggles.aji.xml]</pre>
 *
 * The checks do not depend on whether {@code apps/shared} is a layer of its own, so the test
 * also accepts {@code projects/expert-goggles-shared-layer.aji.xml}.
 *
 * <p>expert-goggles changes from day to day, so the test fixes no file count and no score.
 * It prints them and checks the rules that must hold whatever they are.
 */
public final class ExpertGogglesTest {

    private ExpertGogglesTest() {
    }

    public static void main(String[] args) {
        ProjectRun.test(args, "projects/expert-goggles.aji.xml",
                ExpertGogglesTest::architecture);
    }

    static void architecture(ProjectRun run, Checks c) {
        SystemModel m = run.model();
        c.check("every declared layer holds files",
                m.layerNames().keySet().equals(m.filesByLayer().keySet()),
                "declared " + m.layerNames().keySet() + ", with files "
                        + m.filesByLayer().keySet());

        List<String> tests = run.files(id -> id.contains("/tests/"));
        c.check("the tests are left out, as the project file says", tests.isEmpty(),
                tests.size() + " files in tests folders");

        List<Dep> upward = run.result().edges().upward();
        c.check("no layering violation: every #include stays in its layer or points toward"
                + " the foundation", upward.isEmpty(), ProjectRun.listing(upward));

        List<Dep> acrossLibraries = run.deps(d -> d.from().startsWith("facilities/")
                && d.to().startsWith("facilities/")
                && !ProjectRun.part(d.from(), 1).equals(ProjectRun.part(d.to(), 1)));
        c.check("the libraries of the facilities layer are independent of one another",
                acrossLibraries.isEmpty(), ProjectRun.listing(acrossLibraries));

        List<Dep> acrossApps = run.deps(d -> d.from().startsWith("apps/")
                && d.to().startsWith("apps/")
                && !ProjectRun.part(d.to(), 1).equals("shared")
                && !ProjectRun.part(d.from(), 1).equals(ProjectRun.part(d.to(), 1)));
        c.check("no application includes another, and apps/shared includes none",
                acrossApps.isEmpty(), ProjectRun.listing(acrossApps));

        int modules = run.derived().modules().size();
        c.check("the include graph has no cycle: with derived layers every file is a module"
                + " of its own", modules == m.files().size(),
                modules + " modules for " + m.files().size() + " files");
    }
}
