// Copyright Group W, SPA. All Rights Reserved.
package groupw.LogAdj;

import org.junit.Test;
import java.io.IOException;
import java.util.Map;
import java.util.TreeMap;
import static org.junit.Assert.*;

public class CapabilityGraphGeneratorTest {

    /** Hard-coded paths for testing on known files.
     *
     * In particular, it must be synchronized with the
     * scenario files used in the logistical adjudicator test
     *
     * @throws IOException
     */
    @Test
    public void testParsingAndCalculation() throws IOException {
        CapabilityGraphGenerator generator = new CapabilityGraphGenerator();
        generator.readUnitData("test/Data/unit-C1.csv");
        generator.readSerialData("test/Data/serial-C1.csv");
        generator.parseLogFile("logrun.txt");

        Map<String, TreeMap<Double, Double>> timeline = generator.calculateCumulativeCapability();
        assertNotNull(timeline);
        
        // Check if we have at least some units with deliveries
        boolean foundDeliveries = false;
        for (TreeMap<Double, Double> unitData : timeline.values()) {
            if (unitData.size() > 1) { // 1 is just the (0.0, 0.0) point
                foundDeliveries = true;
                break;
            }
        }
        assertTrue("Expected some units to have deliveries in logrun.txt", foundDeliveries);

        timeline.forEach((unit, data) -> {
            System.out.println("Unit " + unit + ", had "+ data.size()+" data points");
            if (data.size() > 1) {
                double lastCap = data.lastEntry().getValue();
                System.out.printf("Final percent capability: %.3f \n", lastCap);
                System.out.printf("Final time: %.3f \n", data.lastKey()); // kind of redundant, now
                assertTrue("Cumulative capability should be non-decreasing", isNonDecreasing(data));
            }
        });
    }


    @Test
    public void testGenerateUnitGraph() throws IOException {
        CapabilityGraphGenerator generator = new CapabilityGraphGenerator();
        generator.readUnitData("test/Data/unit-C1.csv");
        generator.readSerialData("test/Data/serial-C1.csv");
        generator.parseLogFile("logrun.txt");
        
        String outputPath = "capability_graph_units.png";
        generator.saveUnitsGraph(outputPath);
        
        java.io.File file = new java.io.File(outputPath);
        assertTrue("Graph image should be created", file.exists());
        System.out.println("Units graph image: " + file.getAbsolutePath());
    }


    @Test
    public void testGenerateTotalGraph() throws IOException {
        CapabilityGraphGenerator generator = new CapabilityGraphGenerator();
        generator.readUnitData("test/Data/unit-C1.csv");
        generator.readSerialData("test/Data/serial-C1.csv");
        generator.parseLogFile("logrun.txt");

        String outputPath = "capability_graph_total.png";
        generator.saveTotalGraph(outputPath);

        java.io.File file = new java.io.File(outputPath);
        assertTrue("Graph image should be created", file.exists());
        System.out.println("Total graph image: " + file.getAbsolutePath());
    }

    private boolean isNonDecreasing(TreeMap<Double, Double> data) {
        double prev = -1.0;
        for (double val : data.values()) {
            if (val < prev) return false;
            prev = val;
        }
        return true;
    }
}

// Copyright Group W, SPA. All Rights Reserved.
