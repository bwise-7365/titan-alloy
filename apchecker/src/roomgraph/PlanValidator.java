package roomgraph;

import java.io.IOException;
import java.nio.file.Path;

import org.xml.sax.SAXException;

/**
 * Validates a plan file against plan.xsd before it is parsed.
 *
 * XSD 1.0 covers the structure, the enumerations, and referential integrity
 * between doors and rooms. The three constraints it cannot state remain in the
 * Java constructors, and are listed in the schema's comments:
 *
 *   1. lengthFeet is required for a PASSAGE and forbidden elsewhere;
 *   2. exactly one room has kind EXTERIOR;
 *   3. no unordered pair of rooms carries two doors.
 *
 * Validation is a separate step on purpose. PlanReader still checks everything
 * itself, so the program is correct whether or not a schema is at hand; the
 * schema exists to give the author of a plan a fast, precise error message.
 */
public final class PlanValidator {

    private PlanValidator() {
        // Utility class.
    }

    /** Throws PlanFormatException with the schema's own message on any fault. */
    public static void validate(Path planFile, Path schemaFile) throws PlanFormatException {
        try {
            XmlValidation.validate(planFile, schemaFile);
        } catch (SAXException failure) {
            throw new PlanFormatException(planFile + " does not satisfy " + schemaFile + ": "
                    + failure.getMessage(), failure);
        } catch (IOException failure) {
            throw new PlanFormatException("Could not read " + planFile + ": " + failure.getMessage(), failure);
        }
    }
}
