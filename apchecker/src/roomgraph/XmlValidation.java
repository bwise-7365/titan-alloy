package roomgraph;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import javax.xml.XMLConstants;
import javax.xml.transform.stream.StreamSource;
import javax.xml.validation.Schema;
import javax.xml.validation.SchemaFactory;
import javax.xml.validation.Validator;

import org.xml.sax.SAXException;

/**
 * Validates an XML file against an XSD, using the JDK's own validator.
 *
 * Both the plan and the rule file need this, and the two callers differ only in
 * which exception they raise, so the mechanism lives here once and each caller
 * wraps it in its own vocabulary.
 *
 * External entities and network access are switched off. A plan is data, not a
 * program.
 */
public final class XmlValidation {

    private XmlValidation() {
        // Utility class.
    }

    /**
     * Throws with the schema's own message on any fault. The caller is expected
     * to wrap this in a domain exception.
     */
    public static void validate(Path xmlFile, Path schemaFile) throws SAXException, IOException {
        if (!Files.isReadable(schemaFile)) {
            throw new IOException("Cannot read schema file: " + schemaFile);
        }
        if (!Files.isReadable(xmlFile)) {
            throw new IOException("Cannot read XML file: " + xmlFile);
        }

        SchemaFactory factory = SchemaFactory.newInstance(XMLConstants.W3C_XML_SCHEMA_NS_URI);
        factory.setProperty(XMLConstants.ACCESS_EXTERNAL_DTD, "");
        factory.setProperty(XMLConstants.ACCESS_EXTERNAL_SCHEMA, "file");

        Schema schema = factory.newSchema(schemaFile.toFile());
        Validator validator = schema.newValidator();
        validator.setProperty(XMLConstants.ACCESS_EXTERNAL_DTD, "");
        validator.setProperty(XMLConstants.ACCESS_EXTERNAL_SCHEMA, "");
        validator.validate(new StreamSource(xmlFile.toFile()));
    }
}
