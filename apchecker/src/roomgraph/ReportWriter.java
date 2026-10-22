package roomgraph;

import java.util.List;

/**
 * Writes the verdict vector as XML, for a caller that is a program rather than a
 * person.
 *
 * The plan is XML, the rules are XML, and so the verdict is too. Nothing here
 * interprets the result; the search decides what a broken PATTERN rule is worth
 * against a broken CODE rule, and this file takes no view on that.
 */
public final class ReportWriter {

    private ReportWriter() {
        // Utility class.
    }

    public static String toXml(PlanGraph plan, List<Verdict> verdicts) {
        StringBuilder out = new StringBuilder();
        out.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        out.append("<report xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
        out.append("        xsi:noNamespaceSchemaLocation=\"report.xsd\"\n");
        out.append("        plan=\"").append(escape(plan.name())).append("\"\n");
        out.append("        rooms=\"").append(plan.rooms().size()).append("\"\n");
        out.append("        connections=\"").append(plan.connections().size()).append("\"\n");
        out.append("        satisfied=\"").append(count(verdicts, Outcome.SATISFIED)).append("\"\n");
        out.append("        broken=\"").append(count(verdicts, Outcome.BROKEN)).append("\"\n");
        out.append("        undetermined=\"").append(count(verdicts, Outcome.UNDETERMINED)).append("\">\n");

        for (Verdict verdict : verdicts) {
            appendVerdict(out, verdict);
        }
        out.append("</report>\n");
        return out.toString();
    }

    private static void appendVerdict(StringBuilder out, Verdict verdict) {
        out.append("  <verdict rule=\"").append(escape(verdict.ruleId()))
                .append("\" severity=\"").append(verdict.severity())
                .append("\" result=\"").append(verdict.outcome()).append("\"");
        if (verdict.outcome() != Outcome.BROKEN) {
            out.append("/>\n");
            return;
        }
        out.append(">\n");
        for (Violation violation : verdict.violations()) {
            appendFinding(out, violation);
        }
        out.append("  </verdict>\n");
    }

    private static void appendFinding(StringBuilder out, Violation violation) {
        out.append("    <finding message=\"").append(escape(violation.message())).append("\">\n");
        for (String room : violation.rooms()) {
            out.append("      <room ref=\"").append(escape(room)).append("\"/>\n");
        }
        for (Connection connection : violation.connections()) {
            out.append("      <connection from=\"").append(escape(connection.a()))
                    .append("\" to=\"").append(escape(connection.b()))
                    .append("\" kind=\"").append(connection.kind()).append("\"/>\n");
        }
        out.append("    </finding>\n");
    }

    private static int count(List<Verdict> verdicts, Outcome outcome) {
        int total = 0;
        for (Verdict verdict : verdicts) {
            if (verdict.outcome() == outcome) {
                total++;
            }
        }
        return total;
    }

    private static String escape(String raw) {
        return raw.replace("&", "&amp;")
                .replace("<", "&lt;")
                .replace(">", "&gt;")
                .replace("\"", "&quot;");
    }
}
