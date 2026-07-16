package roomgraph;

import java.io.IOException;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.xml.XMLConstants;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

/**
 * Compares two reports on the same design, and so defines what "conforming"
 * means for an independent implementation of the rule language.
 *
 * Two reports agree when, for every rule, they give the same outcome and - for a
 * broken rule - the same set of findings. Two things deliberately do NOT count as
 * a disagreement:
 *
 *   1. the ORDER of the verdicts, and of the findings within a verdict;
 *   2. the OMISSION of an UNDETERMINED verdict, which a consumer must read as
 *      undetermined anyway.
 *
 * A byte-for-byte diff would forbid both, and would therefore reject a perfectly
 * correct C++ checker for putting its output in a different order. Conformance is
 * a statement about content.
 *
 * <pre>
 *   java roomgraph.ReportCompare expected.xml actual.xml
 * </pre>
 *
 * Exit status 0 when the two agree, 1 when they do not, 2 on a bad file.
 */
public final class ReportCompare {

    /** One rule's outcome, and the findings it raised. */
    private record RuleResult(String outcome, Set<String> messages) {
    }

    private ReportCompare() {
        // Entry point only.
    }

    public static void main(String[] args) {
        if (args.length != 2) {
            System.err.println("Usage: java roomgraph.ReportCompare <expected.xml> <actual.xml>");
            System.exit(2);
            return;
        }
        try {
            List<String> complaints = compare(read(Path.of(args[0])), read(Path.of(args[1])));
            if (complaints.isEmpty()) {
                System.exit(0);
            }
            for (String complaint : complaints) {
                System.out.println("    " + complaint);
            }
            System.exit(1);
        } catch (IOException | SAXException | ParserConfigurationException failure) {
            System.err.println("Could not read a report: " + failure.getMessage());
            System.exit(2);
        }
    }

    /** Empty when the two reports agree; otherwise one line per disagreement. */
    public static List<String> compare(Map<String, RuleResult> expected, Map<String, RuleResult> actual) {
        Set<String> rules = new LinkedHashSet<>(expected.keySet());
        rules.addAll(actual.keySet());

        List<String> complaints = new ArrayList<>();
        for (String rule : rules) {
            RuleResult want = expected.getOrDefault(rule, undetermined());
            RuleResult got = actual.getOrDefault(rule, undetermined());

            if (!want.outcome().equals(got.outcome())) {
                complaints.add(rule + ": expected " + want.outcome() + ", got " + got.outcome());
                continue;
            }
            if (!want.messages().equals(got.messages())) {
                complaints.add(rule + ": same outcome, different findings");
                for (String message : difference(want.messages(), got.messages())) {
                    complaints.add("    missing: " + message);
                }
                for (String message : difference(got.messages(), want.messages())) {
                    complaints.add("    unexpected: " + message);
                }
            }
        }
        return complaints;
    }

    /**
     * A rule absent from a report is undetermined. That is the convention the
     * report schema states, and honoring it here is what lets an implementation
     * omit the verdicts it has nothing to say about.
     */
    private static RuleResult undetermined() {
        return new RuleResult("UNDETERMINED", Set.of());
    }

    private static Set<String> difference(Set<String> from, Set<String> remove) {
        Set<String> only = new LinkedHashSet<>(from);
        only.removeAll(remove);
        return only;
    }

    static Map<String, RuleResult> read(Path file) throws IOException, SAXException, ParserConfigurationException {
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        factory.setFeature(XMLConstants.FEATURE_SECURE_PROCESSING, true);
        factory.setNamespaceAware(false);
        factory.setIgnoringComments(true);
        Document document = factory.newDocumentBuilder().parse(file.toFile());

        Map<String, RuleResult> results = new LinkedHashMap<>();
        for (Element verdict : children(document.getDocumentElement(), "verdict")) {
            Set<String> messages = new LinkedHashSet<>();
            for (Element finding : children(verdict, "finding")) {
                messages.add(finding.getAttribute("message"));
            }
            results.put(verdict.getAttribute("rule"),
                    new RuleResult(verdict.getAttribute("result"), Collections.unmodifiableSet(messages)));
        }
        return results;
    }

    private static List<Element> children(Element parent, String tag) {
        List<Element> found = new ArrayList<>();
        NodeList nodes = parent.getChildNodes();
        for (int i = 0; i < nodes.getLength(); i++) {
            Node node = nodes.item(i);
            if (node.getNodeType() == Node.ELEMENT_NODE && tag.equals(node.getNodeName())) {
                found.add((Element) node);
            }
        }
        return found;
    }
}
