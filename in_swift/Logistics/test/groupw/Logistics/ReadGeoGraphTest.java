/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.Logistics;

import groupw.Network.DTDErrorHandler;
import org.junit.Test;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import java.io.IOException;

import static groupw.Logistics.ReadGeoGraph.ARCS;
import static groupw.Logistics.ReadGeoGraph.NODES;
import static groupw.Network.NWUtils.simpleReadFile;
import static groupw.Network.NWUtils.usableCurrDirPath;
import static org.junit.Assert.assertEquals;

/**
 * @author BenWise
 */
public class ReadGeoGraphTest {

    public ReadGeoGraphTest() {
    }

    @Test
    public void testReadRRStructure() throws Exception {
        System.out.println("\nStarting testReadRRStructure");
    }

    /**
     * Check that the named file is valid according to the DTD
     */
    @Test
    public void testDTD02() {
        String testFile = "StalingradRR.xml";
        testDTD(testFile, false);
    }

    public void testDTD(String testFile, boolean expFailP) {
        System.out.printf("\nTesting DTD validation of %s\n", testFile);
        if (expFailP) {
            System.out.println("It should fail validation and produce errors.");
        } else {
            System.out.println("No errors are expected.");
        }

        String fDir = usableCurrDirPath() + "\\rsrcs\\";
        DTDErrorHandler eh = new DTDErrorHandler();
        try {
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            factory.setValidating(true);
            factory.setNamespaceAware(true);
            DocumentBuilder loader = factory.newDocumentBuilder();

            loader.setErrorHandler(eh);

            Document dcmnt = loader.parse(fDir + testFile);
            dcmnt.getDocumentElement().normalize();

        } catch (ParserConfigurationException | IOException | SAXException ex) {
            System.out.println("ERROR : " + ex.getMessage());
        }
        assertEquals(expFailP, eh.dtdValidationFailed);
    }

    @Test
    public void testSimpleReadFileV01() {
        System.out.println("\nStarting testSimpleReadFileV01");
        // create BufferedReader for terminal input
        //BufferedReader bfri = new BufferedReader( new InputStreamReader(System.in));

        String fDir = usableCurrDirPath() + "\\rsrcs\\";
        String p1 = fDir + "rr1.json";
        String r1 = simpleReadFile(p1);
        if (null != r1) {
            System.out.printf("File path '%s' successfully read.\n", p1);
            String hBar = "----------";
            System.out.printf("Resulting string:\n%s\n%s\n%s\n",
                    hBar, r1, hBar);
        }
    }

    /**
     * Parse a test file and verify the right number of parts and sub parts.
     *
     */
    @Test
    public void domV3()
            throws SAXException, ParserConfigurationException, IOException, IllegalArgumentException {

        String testFile = "testRR.xml";
        System.out.printf("\nTesting read and parse of %s\n", testFile);

        // current directory is something like ...\stalingrad_2025_src
        String fDir = usableCurrDirPath() + "\\rsrcs\\";
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        DocumentBuilder loader = factory.newDocumentBuilder();
        Document dcmnt = loader.parse(fDir + testFile);
        dcmnt.getDocumentElement().normalize();

        System.out.println("Root element: " + dcmnt.getDocumentElement().getNodeName());

        // TODO: we parse the RailLine twice: in the Arcs and again in Nodes.
        // This repeated code should be combined into a sub-function.
        System.out.println("\nParsing arcs ...");
        NodeList lineListSection = dcmnt.getElementsByTagName(ARCS);
        int numLL = lineListSection.getLength();
        assert (1 == numLL); // only one ARCS section
        Element llElem = (Element) lineListSection.item(0);
        //parseRRLineList(llElem);
        System.out.flush();

        System.out.println("\nParsing nodes ...");
        NodeList nodeListSection = dcmnt.getElementsByTagName(NODES);
        int nIS = nodeListSection.getLength();
        assertEquals(1, nIS); // only one NODES section
        Element nlElem = (Element) nodeListSection.item(0);
        //parseRRNodeList(nlElem);
        System.out.flush();

    }

}
// =============================================================================
