// Copyright Group W, SPA. All Rights Reserved.

package groupw.LogAdj;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.*;

import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartUtilities;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYStepRenderer;
import java.awt.BasicStroke;
import java.io.File;


/**
 * Draw a basic multi-line graph of delivered unit capability over time.
 *
 * The key part is to keep track of the relationship between serials and their parent
 * unit, so we can determine whether they have arrived at the destination, and
 * how much of the unit's capability each serial represents.
 */
public class CapabilityGraphGenerator {
    private Map<String, String> serialToUnit = new HashMap<>();
    private Map<String, Double> serialToCapability = new HashMap<>();
    private Map<String, String> unitToDeliveryNode = new HashMap<>();
    private Map<String, List<DeliveryEvent>> unitDeliveries = new HashMap<>();
    private double maxTime = 0.0;

    public static class DeliveryEvent {
        double time;
        double capability;

        public DeliveryEvent(double time, double capability) {
            this.time = time;
            this.capability = capability;
        }

        @Override
        public String toString() {
            return "DeliveryEvent{" + "time=" + time + ", capability=" + capability + '}';
        }
    }

    public void readUnitData(String path) throws IOException {
        try (BufferedReader br = new BufferedReader(new FileReader(path))) {
            String line;
            String[] headers = null;
            while ((line = br.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#")) continue;
                String[] parts = line.split(",");
                if (headers == null) {
                    headers = new String[parts.length];
                    for (int i = 0; i < parts.length; i++) headers[i] = parts[i].trim();
                } else {
                    String unitName = null;
                    String deliveryNode = null;
                    for (int i = 0; i < Math.min(parts.length, headers.length); i++) {
                        if (headers[i].equalsIgnoreCase("Unit Name")) unitName = parts[i].trim();
                        if (headers[i].equalsIgnoreCase("Delivery Node")) deliveryNode = parts[i].trim();
                    }
                    if (unitName != null && deliveryNode != null) {
                        unitToDeliveryNode.put(unitName, deliveryNode);
                    }
                }
            }
        }
    }

    public void readSerialData(String path) throws IOException {
        try (BufferedReader br = new BufferedReader(new FileReader(path))) {
            String line;
            String[] headers = null;
            while ((line = br.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#")) continue;
                String[] parts = line.split(",");
                if (headers == null) {
                    headers = new String[parts.length];
                    for (int i = 0; i < parts.length; i++) headers[i] = parts[i].trim();
                } else {
                    String serialName = null;
                    String unitName = null;
                    Double capability = null;
                    for (int i = 0; i < Math.min(parts.length, headers.length); i++) {
                        if (headers[i].equalsIgnoreCase("Serial Name")) serialName = parts[i].trim();
                        if (headers[i].equalsIgnoreCase("Unit Name")) unitName = parts[i].trim();
                        if (headers[i].equalsIgnoreCase("Percent Capability")) capability = Double.parseDouble(parts[i].trim());
                    }
                    if (serialName != null && unitName != null && capability != null) {
                        serialToUnit.put(serialName, unitName);
                        serialToCapability.put(serialName, capability);
                    }
                }
            }
        }
    }

    public void parseLogFile(String path) throws IOException {
        try (BufferedReader br = new BufferedReader(new FileReader(path))) {
            String line;
            while ((line = br.readLine()) != null) {
                String[] parts = line.split(",");
                if (parts.length > 0) {
                    try {
                        double time = Double.parseDouble(parts[0].replace(" hours", "").trim());
                        if (time > maxTime) maxTime = time;
                    } catch (NumberFormatException e) {
                        // ignore
                    }
                }

                if (line.contains("Serial") && line.contains("finish, unload")) {
                    if (parts.length >= 7) {
                        try {
                            double time = Double.parseDouble(parts[0].replace(" hours", "").trim());
                            String serialName = parts[2].trim();
                            String action = parts[3].trim();
                            String subAction = parts[4].trim();
                            String location = parts[6].trim();

                            if (action.equals("finish") && subAction.equals("unload")) {
                                String unitName = serialToUnit.get(serialName);
                                if (unitName != null) {
                                    String targetNode = unitToDeliveryNode.get(unitName);
                                    if (location.equals(targetNode)) {
                                        double cap = serialToCapability.getOrDefault(serialName, 0.0);
                                        unitDeliveries.computeIfAbsent(unitName, k -> new ArrayList<>())
                                                .add(new DeliveryEvent(time, cap));
                                    }
                                }
                            }
                        } catch (NumberFormatException e) {
                            // Skip lines with invalid time format if any
                        }
                    }
                }
            }
        }
    }

    public Map<String, TreeMap<Double, Double>> calculateCumulativeCapability() {
        double tolerance = 1E-6;
        Map<String, TreeMap<Double, Double>> unitTimeline = new HashMap<>();
        for (String unit : unitToDeliveryNode.keySet()) {
            List<DeliveryEvent> events = unitDeliveries.getOrDefault(unit, new ArrayList<>());
            events.sort(Comparator.comparingDouble(e -> e.time));

            TreeMap<Double, Double> timeline = new TreeMap<>();
            double cumulative = 0;
            timeline.put(0.0, 0.0);

            for (DeliveryEvent event : events) {
                cumulative += event.capability;
                if (100+tolerance < cumulative) {
                    // this really should have been checked at data-construction time
                    throw new RuntimeException("Cumulative capability over 100%: "+ cumulative);
                }
                timeline.put(event.time, cumulative);
            }
            // Extend the line to the end of the simulation
            if (maxTime > 0) {
                timeline.put(maxTime, cumulative);
            }
            unitTimeline.put(unit, timeline);
        }
        return unitTimeline;
    }

    public Map<String, List<DeliveryEvent>> getUnitDeliveries() {
        return unitDeliveries;
    }

    public JFreeChart createChart() {
        XYSeriesCollection dataset = new XYSeriesCollection();
        Map<String, TreeMap<Double, Double>> unitTimeline = calculateCumulativeCapability();

        // Sort unit names for consistent legend order
        List<String> sortedUnits = new ArrayList<>(unitTimeline.keySet());
        Collections.sort(sortedUnits);

        for (String unit : sortedUnits) {
            XYSeries series = new XYSeries(unit);
            unitTimeline.get(unit).forEach(series::add);
            dataset.addSeries(series);
        }

        JFreeChart chart = ChartFactory.createXYLineChart(
                "Cumulative Capability Delivered",
                "Simulation Time (hours)",
                "Capability (%)",
                dataset,
                PlotOrientation.VERTICAL,
                true, true, false);


        float lineWidth = 2.5f; // 'float' required to avoid compiler complaints
        XYPlot plot = chart.getXYPlot();
        XYStepRenderer renderer = new XYStepRenderer();
        BasicStroke stroke = new BasicStroke(lineWidth);
        for (int i = 0; i < dataset.getSeriesCount(); i++) {
            renderer.setSeriesStroke(i, stroke);
        }
        plot.setRenderer(renderer);

        return chart;
    }

    public void saveGraph(String filePath) throws IOException {
        JFreeChart chart = createChart();
        File imageFile = new File(filePath);
        System.out.println("Graph will be saved to: " + imageFile.getAbsolutePath());
        ChartUtilities.saveChartAsPNG(imageFile, chart, 1200, 600);
    }

    /**
     * This testing function has to be kept synchronized with
     * the logistical adjudicator test scenario
     * @param args
     */
    public static void main(String[] args) {
        CapabilityGraphGenerator generator = new CapabilityGraphGenerator();
        try {
            generator.readUnitData("test/Data/unit-C1.csv");
            generator.readSerialData("test/Data/serial-C1.csv");
            generator.parseLogFile("logrun.txt");
            generator.saveGraph("capability_graph.png");

        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}

// Copyright Group W, SPA. All Rights Reserved.
