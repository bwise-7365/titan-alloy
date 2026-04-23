/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */

package groupw.Logistics;

import java.io.IOException;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import org.w3c.dom.Document;
import org.xml.sax.SAXException;

/**
 *
 * @author BenWise
 */
abstract public class ReadGeoGraph {

    // record some constants needed for parsing XML
    public static final String ARCS = "Arcs";
    public static final String NODES = "Nodes";
    /**
     * 
     * Using the basic javax.xml.parsers, read XML into a document
     * 
     * @param rrDataFilePath
     * @throws SAXException
     * @throws ParserConfigurationException
     * @throws IOException
     * @throws IllegalArgumentException 
     */
    static public void readRRStructure(String rrDataFilePath)
            throws SAXException, ParserConfigurationException, IOException, IllegalArgumentException {

        System.out.printf("\nTesting read and parse of %s\n", rrDataFilePath);
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        DocumentBuilder loader = factory.newDocumentBuilder();
        Document dcmnt = loader.parse(rrDataFilePath);
        dcmnt.getDocumentElement().normalize();
        System.out.println("Root element: " + dcmnt.getDocumentElement().getNodeName());
    }

}


// =============================================================================