/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.Logistics;

import com.jsevy.jsvg.SVGDocument;
import org.jgrapht.alg.connectivity.ConnectivityInspector;
import org.junit.Test;
import java.io.IOException;
import java.util.List;
import java.util.Set;

import static groupw.Logistics.GeoGraph.makeTestGraphA;
import static org.junit.Assert.*;

/**
 *
 * @author BenWise
 */
public class DrawGeoGraphTest {
    
    public DrawGeoGraphTest() {
    }

    @Test
    public void testCreateSVG() throws IOException {
        System.out.println("\nStarting testCreateSVG");

        // US Letter size, Landscape orientation, scaled
        double inchWidth = 11.0;
        double inchHeight = 8.5;
        double inchMargin = 0.5;
        GeoGraph g = makeTestGraphA();
        assertNotNull(g);
        assertNotNull(g.wpg);

        assertEquals(11, g.wpg.vertexSet().size());
        assertEquals(20, g.wpg.edgeSet().size());

        ConnectivityInspector<GeoGraph.GeoNode, GeoGraph.GeoEdge> inspector = new ConnectivityInspector<>(g.wpg);
        List<Set<GeoGraph.GeoNode>> ccs = inspector.connectedSets();
        int numCC = ccs.size();

        assertEquals(2, numCC);

        DrawGeoGraph dgg = new DrawGeoGraph(g);
        SVGDocument svgDoc = dgg.drawSVG(inchWidth, inchHeight, inchMargin);
        String filePath = "./tmp_file.svg";
        dgg.outputSVGtoFile(svgDoc, filePath);
    }
}
// =============================================================================
