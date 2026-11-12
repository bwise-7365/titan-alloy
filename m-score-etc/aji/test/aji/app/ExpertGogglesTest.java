package aji.app;

import aji.model.Dep;
import aji.model.SystemModel;
import aji.testing.Checks;

import java.util.List;

/**
 * Runs aji on expert-goggles and checks the result against the architecture that
 * expert-goggles documents: layers whose dependencies always point toward the foundation
 * ({@code restructure/README.md}), libraries in the facilities layer that are independent of
 * one another, shared support in {@code appsupport/} that uses no application, and
 * applications that never use another application ({@code apps/README.md}), each with its
 * headers in {@code apps/NAME/include/NAME/}. Run it from the aji folder, or pass the project
 * file:
 *
 * <pre>  java -cp out aji.app.ExpertGogglesTest [projects/expert-goggles.aji.xml]</pre>
 *
 * The checks do not depend on how finely the layers are declared, so the test also accepts
 * {@code projects/expert-goggles-shared-layer.aji.xml}, which declares one layer for each
 * of the six folders.
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
                && !ProjectRun.part(d.from(), 1).equals(ProjectRun.part(d.to(), 1)));
        c.check("no application includes another", acrossApps.isEmpty(),
                ProjectRun.listing(acrossApps));

        List<Dep> supportUsesApps = run.deps(d -> d.from().startsWith("appsupport/")
                && d.to().startsWith("apps/"));
        c.check("the support that the applications share (appsupport) includes no application",
                supportUsesApps.isEmpty(), ProjectRun.listing(supportUsesApps));

        List<String> strayHeaders = run.files(id -> id.startsWith("apps/")
                && (id.endsWith(".hpp") || id.endsWith(".h"))
                && !id.startsWith("apps/" + ProjectRun.part(id, 1) + "/include/"
                        + ProjectRun.part(id, 1) + "/"));
        c.check("the headers of each application are in its folder include/NAME",
                strayHeaders.isEmpty(), strayHeaders.size() + " elsewhere"
                        + (strayHeaders.isEmpty() ? "" : ", the first " + strayHeaders.get(0)));

        int modules = run.derived().modules().size();
        c.check("the include graph has no cycle: with derived layers every file is a module"
                + " of its own", modules == m.files().size(),
                modules + " modules for " + m.files().size() + " files");
    }
}
