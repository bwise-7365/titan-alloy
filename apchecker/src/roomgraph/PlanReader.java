package roomgraph;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.OptionalDouble;

import javax.xml.XMLConstants;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

/**
 * Reads a plan from XML using nothing but the JDK's own DOM parser.
 *
 * Expected shape:
 *
 * <pre>
 * &lt;plan name="Example"&gt;
 *   &lt;rooms&gt;
 *     &lt;room id="outside" label="Outdoors" kind="EXTERIOR" privacy="EXTERIOR"/&gt;
 *     &lt;room id="hall"    label="Hall"     kind="PASSAGE"  privacy="PUBLIC" lengthFeet="18"/&gt;
 *   &lt;/rooms&gt;
 *   &lt;connections&gt;
 *     &lt;connection from="outside" to="hall"/&gt;
 *   &lt;/connections&gt;
 * &lt;/plan&gt;
 * </pre>
 *
 * Every attribute is required except {@code lengthFeet}, which is required on a
 * PASSAGE and forbidden elsewhere. A missing or unrecognized value raises an
 * exception; nothing is defaulted, because a plan that silently checks out
 * against invented data is worse than no check at all.
 */
public final class PlanReader {

    private PlanReader() {
        // Utility class.
    }

    public static PlanGraph read(Path file) throws PlanFormatException {
        if (!Files.isReadable(file)) {
            throw new PlanFormatException("Cannot read plan file: " + file);
        }
        Document document = parse(file);
        Element root = document.getDocumentElement();
        if (!"plan".equals(root.getTagName())) {
            throw new PlanFormatException("Root element must be <plan>, found <" + root.getTagName() + ">.");
        }
        String name = requiredAttribute(root, "name");

        List<Room> rooms = readRooms(root);
        List<Connection> connections = readDoors(root);

        try {
            return new PlanGraph(name, rooms, connections);
        } catch (IllegalArgumentException failure) {
            throw new PlanFormatException("The plan is inconsistent: " + failure.getMessage(), failure);
        }
    }

    private static Document parse(Path file) throws PlanFormatException {
        try {
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            // Refuse external entities: a plan file is data, not a program.
            factory.setFeature(XMLConstants.FEATURE_SECURE_PROCESSING, true);
            factory.setFeature("http://apache.org/xml/features/disallow-doctype-decl", true);
            factory.setNamespaceAware(false);
            factory.setIgnoringComments(true);
            DocumentBuilder builder = factory.newDocumentBuilder();
            return builder.parse(file.toFile());
        } catch (ParserConfigurationException | SAXException | IOException failure) {
            throw new PlanFormatException("Could not parse " + file + ": " + failure.getMessage(), failure);
        }
    }

    private static List<Room> readRooms(Element root) throws PlanFormatException {
        List<Room> rooms = new ArrayList<>();
        List<Element> holder = childElements(root, "rooms");
        if (holder.isEmpty()) {
            return rooms;
        }
        for (Element element : childElements(holder.get(0), "room")) {
            rooms.add(readRoom(element));
        }
        // A plan may declare no rooms. The empty design - no rooms, no connections,
        // not even the outdoors - is the root of the search, and every rule must
        // be able to return "cannot yet tell" about it rather than choke.
        return rooms;
    }

    private static Room readRoom(Element element) throws PlanFormatException {
        String id = requiredAttribute(element, "id");
        String label = requiredAttribute(element, "label");
        RoomKind kind = parseKind(requiredAttribute(element, "kind"), id);
        PrivacyLevel privacy = parsePrivacy(requiredAttribute(element, "privacy"), id);

        OptionalDouble lengthFeet = OptionalDouble.empty();
        if (element.hasAttribute("lengthFeet")) {
            String raw = element.getAttribute("lengthFeet");
            try {
                lengthFeet = OptionalDouble.of(Double.parseDouble(raw));
            } catch (NumberFormatException failure) {
                throw new PlanFormatException("Room '" + id + "': lengthFeet='" + raw + "' is not a number.", failure);
            }
        }

        java.util.Optional<ExteriorKind> outdoor = java.util.Optional.empty();
        if (element.hasAttribute("outdoor")) {
            outdoor = java.util.Optional.of(parseExteriorKind(element.getAttribute("outdoor"), id));
        }

        try {
            return new Room(id, label, kind, privacy, lengthFeet, outdoor);
        } catch (IllegalArgumentException failure) {
            throw new PlanFormatException(failure.getMessage(), failure);
        }
    }

    private static List<Connection> readDoors(Element root) throws PlanFormatException {
        List<Connection> connections = new ArrayList<>();
        List<Element> holder = childElements(root, "connections");
        if (holder.isEmpty()) {
            return connections;
        }
        for (Element element : childElements(holder.get(0), "connection")) {
            String from = requiredAttribute(element, "from");
            String to = requiredAttribute(element, "to");
            ConnectionKind kind = parseConnectionKind(requiredAttribute(element, "kind"), from, to);
            try {
                connections.add(new Connection(from, to, kind));
            } catch (IllegalArgumentException failure) {
                throw new PlanFormatException(failure.getMessage(), failure);
            }
        }
        // A plan may declare no connections. That is not a malformed file; it is a
        // partial design in which nothing has been joined up yet, and the rules
        // must be able to give a verdict on it like any other.
        return connections;
    }

    private static ConnectionKind parseConnectionKind(String raw, String from, String to)
            throws PlanFormatException {
        try {
            return ConnectionKind.valueOf(raw);
        } catch (IllegalArgumentException failure) {
            throw new PlanFormatException("Connection " + from + "-" + to + ": unknown kind '" + raw
                    + "'. Use DOOR or OPENING.", failure);
        }
    }

    private static ExteriorKind parseExteriorKind(String raw, String roomId) throws PlanFormatException {
        try {
            return ExteriorKind.valueOf(raw);
        } catch (IllegalArgumentException failure) {
            throw new PlanFormatException("Room '" + roomId + "': unknown outdoor kind '" + raw + "'.", failure);
        }
    }

    private static RoomKind parseKind(String raw, String roomId) throws PlanFormatException {
        try {
            return RoomKind.valueOf(raw);
        } catch (IllegalArgumentException failure) {
            throw new PlanFormatException("Room '" + roomId + "': unknown kind '" + raw + "'.", failure);
        }
    }

    private static PrivacyLevel parsePrivacy(String raw, String roomId) throws PlanFormatException {
        try {
            return PrivacyLevel.valueOf(raw);
        } catch (IllegalArgumentException failure) {
            throw new PlanFormatException("Room '" + roomId + "': unknown privacy level '" + raw + "'.", failure);
        }
    }

    private static String requiredAttribute(Element element, String name) throws PlanFormatException {
        if (!element.hasAttribute(name)) {
            throw new PlanFormatException("<" + element.getTagName() + "> is missing the required attribute '" + name + "'.");
        }
        String value = element.getAttribute(name);
        if (value.isBlank()) {
            throw new PlanFormatException("<" + element.getTagName() + "> has a blank '" + name + "' attribute.");
        }
        return value;
    }

    private static Element requiredChild(Element parent, String tag) throws PlanFormatException {
        List<Element> found = childElements(parent, tag);
        if (found.size() != 1) {
            throw new PlanFormatException("<" + parent.getTagName() + "> must contain exactly one <" + tag + ">, found " + found.size() + ".");
        }
        return found.get(0);
    }

    private static List<Element> childElements(Element parent, String tag) {
        List<Element> found = new ArrayList<>();
        NodeList children = parent.getChildNodes();
        for (int i = 0; i < children.getLength(); i++) {
            Node node = children.item(i);
            if (node.getNodeType() == Node.ELEMENT_NODE && tag.equals(node.getNodeName())) {
                found.add((Element) node);
            }
        }
        return found;
    }
}
