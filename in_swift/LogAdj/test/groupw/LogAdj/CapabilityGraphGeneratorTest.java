package groupw.LogAdj;

import org.junit.Test;
import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import static org.junit.Assert.*;

public class CapabilityGraphGeneratorTest {
    @Test
    public void testParsingAndCalculation() throws IOException {
        CapabilityGraphGenerator generator = new CapabilityGraphGenerator();
        generator.readUnitData("test/Data/unit-B2.csv");
        generator.readSerialData("test/Data/serial-B2.csv");
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
            System.out.println("[DEBUG_LOG] Unit: " + unit + ", Data points: " + data.size());
            if (data.size() > 1) {
                double lastCap = data.lastEntry().getValue();
                System.out.println("[DEBUG_LOG]   Final Capability: " + lastCap + "%");
                System.out.println("[DEBUG_LOG]   Final Time: " + data.lastKey());
                assertTrue("Cumulative capability should be non-decreasing", isNonDecreasing(data));
            }
        });
    }

    @Test
    public void testGenerateGraphImage() throws IOException {
        CapabilityGraphGenerator generator = new CapabilityGraphGenerator();
        generator.readUnitData("test/Data/unit-B2.csv");
        generator.readSerialData("test/Data/serial-B2.csv");
        generator.parseLogFile("logrun.txt");
        
        String outputPath = "capability_graph_test.png";
        generator.saveChartAsPNG(outputPath);
        
        java.io.File file = new java.io.File(outputPath);
        assertTrue("Graph image should be created", file.exists());
        System.out.println("[DEBUG_LOG] Test-generated graph image available at: " + file.getAbsolutePath());
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
