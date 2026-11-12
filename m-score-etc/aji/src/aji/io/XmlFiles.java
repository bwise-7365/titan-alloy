package aji.io;

import aji.model.Dep;
import aji.model.FileNode;
import aji.model.SystemModel;
import aji.spec.Glob;
import aji.spec.Language;
import aji.spec.LayerRule;
import aji.spec.ProjectSpec;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import javax.xml.XMLConstants;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

/**
 * Reads the two input formats and writes the dependency model.
 *
 * <p>A dependency model ({@code <system>}) lists files with layers and the dependencies among
 * them. A project file ({@code <aji-project>}) says where the source is and how it is layered;
 * aji extracts the dependencies itself. The README describes both formats.
 */
public final class XmlFiles {

    private XmlFiles() {
    }

    public static String rootName(Path p) throws IOException {
        return parse(p).getDocumentElement().getTagName();
    }

    public static SystemModel readSystem(Path p) throws IOException {
        Element root = parse(p).getDocumentElement();
        expect(root, "system", p);
        List<FileNode> files = new ArrayList<>();
        for (Element e : children(root, "file")) {
            String layer = e.getAttribute("layer");
            files.add(new FileNode(required(e, "id", p),
                    layer.isBlank() ? FileNode.UNASSIGNED : integer(layer, "layer", p)));
        }
        List<String> tests = new ArrayList<>();
        for (Element e : children(root, "test")) {
            tests.add(required(e, "id", p));
        }
        Set<Dep> deps = new LinkedHashSet<>();
        Set<Dep> testDeps = new LinkedHashSet<>();
        for (Element e : children(root, "dep")) {
            String from = required(e, "from", p);
            String to = required(e, "to", p);
            if (!from.equals(to)) {
                ("true".equals(e.getAttribute("test")) ? testDeps : deps).add(new Dep(from, to));
            }
        }
        Map<Integer, String> names = new TreeMap<>();
        for (Element e : children(root, "layer")) {
            names.put(integer(required(e, "number", p), "number", p), e.getAttribute("name"));
        }
        try {
            return new SystemModel(root.getAttribute("name"), files, deps, names, tests, testDeps);
        } catch (IllegalArgumentException ex) {
            throw new IOException(p + ": " + ex.getMessage(), ex);
        }
    }

    public static ProjectSpec readProject(Path p) throws IOException {
        Element root = parse(p).getDocumentElement();
        expect(root, "aji-project", p);
        Path base = p.toAbsolutePath().getParent();
        Path projectRoot = base.resolve(required(root, "root", p)).normalize();
        Language language;
        try {
            language = Language.parse(required(root, "language", p));
        } catch (IllegalArgumentException ex) {
            throw new IOException(p + ": " + ex.getMessage(), ex);
        }
        List<String> sources = new ArrayList<>();
        children(root, "source").forEach(e -> sources.add(e.getAttribute("dir")));
        List<String> includes = new ArrayList<>();
        children(root, "include").forEach(e -> includes.add(e.getAttribute("dir")));
        List<Glob> excludes = new ArrayList<>();
        for (Element e : children(root, "exclude")) {
            excludes.add(Glob.of(required(e, "glob", p)));
        }
        List<Glob> tests = new ArrayList<>();
        for (Element e : children(root, "tests")) {
            tests.add(Glob.of(required(e, "glob", p)));
        }
        List<LayerRule> layers = new ArrayList<>();
        for (Element e : children(root, "layer")) {
            List<Glob> globs = new ArrayList<>();
            for (Element g : children(e, "path")) {
                globs.add(Glob.of(required(g, "glob", p)));
            }
            if (globs.isEmpty()) {
                throw new IOException(p + ": <layer> needs at least one <path glob=\"...\"/>");
            }
            layers.add(new LayerRule(integer(required(e, "number", p), "number", p),
                    e.getAttribute("name"), globs));
        }
        String name = root.getAttribute("name");
        if (name.isBlank()) {
            name = projectRoot.getFileName() == null ? "project" : projectRoot.getFileName().toString();
        }
        boolean defaults = !"false".equalsIgnoreCase(root.getAttribute("default-excludes").trim());
        return new ProjectSpec(name, projectRoot, language, sources, includes, excludes, defaults,
                tests, layers);
    }

    /** Writes a model in the {@code <system>} format, sorted so that runs can be compared. */
    public static void writeSystem(SystemModel m, Path out, String comment) throws IOException {
        StringBuilder b = new StringBuilder();
        b.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        if (comment != null && !comment.isBlank()) {
            b.append("<!-- ").append(comment.replace("--", "- -")).append(" -->\n");
        }
        b.append("<system name=\"").append(esc(m.name())).append("\">\n");
        m.layerNames().forEach((k, v) -> b.append("  <layer number=\"").append(k)
                .append("\" name=\"").append(esc(v)).append("\"/>\n"));
        List<FileNode> files = new ArrayList<>(m.files());
        files.sort((x, y) -> x.layer() != y.layer() ? Integer.compare(x.layer(), y.layer())
                : x.id().compareTo(y.id()));
        for (FileNode f : files) {
            b.append("  <file id=\"").append(esc(f.id())).append("\" layer=\"").append(f.layer())
                    .append("\"/>\n");
        }
        List<Dep> deps = new ArrayList<>(m.deps());
        deps.sort(Dep.ORDER);
        for (Dep d : deps) {
            b.append("  <dep from=\"").append(esc(d.from())).append("\" to=\"").append(esc(d.to()))
                    .append("\"/>\n");
        }
        List<String> tests = new ArrayList<>(m.tests());
        tests.sort(null);
        for (String t : tests) {
            b.append("  <test id=\"").append(esc(t)).append("\"/>\n");
        }
        List<Dep> testDeps = new ArrayList<>(m.testDeps());
        testDeps.sort(Dep.ORDER);
        for (Dep d : testDeps) {
            b.append("  <dep from=\"").append(esc(d.from())).append("\" to=\"").append(esc(d.to()))
                    .append("\" test=\"true\"/>\n");
        }
        b.append("</system>\n");
        Files.writeString(out, b.toString(), StandardCharsets.UTF_8);
    }

    static String esc(String s) {
        return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
                .replace("\"", "&quot;");
    }

    private static Document parse(Path p) throws IOException {
        try {
            DocumentBuilderFactory f = DocumentBuilderFactory.newInstance();
            f.setFeature(XMLConstants.FEATURE_SECURE_PROCESSING, true);
            f.setFeature("http://apache.org/xml/features/nonvalidating/load-external-dtd", false);
            f.setExpandEntityReferences(false);
            f.setNamespaceAware(false);
            return f.newDocumentBuilder().parse(p.toFile());
        } catch (ParserConfigurationException | SAXException ex) {
            throw new IOException(p + ": not readable XML: " + ex.getMessage(), ex);
        }
    }

    private static void expect(Element root, String tag, Path p) throws IOException {
        if (!root.getTagName().equals(tag)) {
            throw new IOException(p + ": expected <" + tag + ">, found <" + root.getTagName() + ">");
        }
    }

    private static List<Element> children(Element parent, String tag) {
        List<Element> out = new ArrayList<>();
        NodeList nodes = parent.getChildNodes();
        for (int k = 0; k < nodes.getLength(); k++) {
            Node n = nodes.item(k);
            if (n instanceof Element e && e.getTagName().equals(tag)) {
                out.add(e);
            }
        }
        return out;
    }

    private static String required(Element e, String attr, Path p) throws IOException {
        String v = e.getAttribute(attr);
        if (v.isBlank()) {
            throw new IOException(p + ": <" + e.getTagName() + "> needs the attribute " + attr);
        }
        return v.trim();
    }

    private static int integer(String s, String attr, Path p) throws IOException {
        try {
            return Integer.parseInt(s.trim());
        } catch (NumberFormatException ex) {
            throw new IOException(p + ": " + attr + "=\"" + s + "\" is not a whole number", ex);
        }
    }
}
