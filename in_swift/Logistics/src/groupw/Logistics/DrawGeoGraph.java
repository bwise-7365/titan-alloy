/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.Logistics;

import groupw.Network.NWUtils.Tuple4;
import com.jsevy.jsvg.SVGDocument;

import groupw.Logistics.GeoGraph.GeoEdge;
import groupw.Logistics.GeoGraph.GeoNode;

import java.awt.*;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Set;

/**
 * Basic tool to draw a GeoGraph in SVG
 *
 * @author BenWise
 */
public class DrawGeoGraph {

    public final double PointsPerInch = 72.0; // by Java 8 standard
    GeoGraph myGraph = null;

    public DrawGeoGraph(GeoGraph g) {
        myGraph = g;
    }

    private final Color[] colorMap = {
        Color.decode("0xcc831d"), // 1: Brown
        Color.decode("0xFF0000"), // 1: Red
        Color.decode("0x0000FF"), // 2: Blue
        Color.decode("0x000000"), // 3: Black
        Color.decode("0xFFA500"), // 4: Orange
        Color.decode("0x00FF00"), // 5: Green
        Color.decode("0xFFFFEE"), // 6: pale beige
        Color.decode("0x808080") // 7: medium gray
    };

    public SVGDocument drawSVG(double inchWidth, double inchHeight, double inchMargin) {

        SVGDocument svgDoc = new SVGDocument();
        svgDoc.setDocumentSize(inchWidth, inchHeight);
        Graphics2D g2d = (Graphics2D) svgDoc.getGraphics();

        // above this size, draw only edges
        int maxSizeLabeled = 250;

        Color dotColor = colorMap[4];
        int dotDiameter = 10; // Points
        Color edgeColor = colorMap[2];
        int edgeWidth = 2; // also Points

        // outline the page itself, filled with white
        drawMarginBox(inchWidth, inchHeight, 0, Color.BLACK,
                true, Color.WHITE, g2d);

        // outline the margins of the map (often with extra on bottom or right) filled with beige
        // notice that we draw the box with HALF the margin of the graph itself,
        // so that nothing hangs off the edge.
        drawMarginBox(inchWidth, inchHeight, inchMargin / 2.0, Color.BLACK,
                true, colorMap[6], g2d);

        Tuple4<Double, Double, Double, Double> bounds = graphBounds();
        double minGX = bounds.get0();
        double minGY = bounds.get1();
        double maxGX = bounds.get2();
        double maxGY = bounds.get3();

        // screen X coordinates increase from left to right, in Points
        // maxSX = ax * maxGX + bx
        // minSX = ax * minGX + bx
        double minSX = PointsPerInch * inchMargin;
        double maxSX = PointsPerInch * (inchWidth - inchMargin);
        double ax = (maxSX - minSX) / (maxGX - minGX);
        double bx = ((maxSX + minSX) - ax * (maxGX + minGX)) / 2.0;

        // screen Y coordinates are opposite of graph coordinates.
        // screen Y coordinates increase from top to bottom, in Points
        // maxSY = ay * minGY + bY
        // minSY = ay * maxGY + bY
        double minSY = PointsPerInch * inchMargin;
        double maxSY = PointsPerInch * (inchHeight - inchMargin);
        double ay = (maxSY - minSY) / (minGY - maxGY);
        double by = ((maxSY + minSY) - (ay * (minGY + maxGY))) / 2.0;

        // set graphics for edges
        g2d.setColor(edgeColor);
        Stroke solidNarrow = new BasicStroke(edgeWidth,
                BasicStroke.CAP_BUTT, // end caps
                BasicStroke.JOIN_BEVEL, // line joins
                0
        );

        // draw all the edges
        Set<GeoEdge> edges = myGraph.wpg.edgeSet();
        for (GeoEdge e0 : edges) {
            GeoNode gns = e0.getSrc();
            GeoNode gnd = e0.getDst();
            int idS = gns.getID();
            int idD = gnd.getID();
            if (idS != idD) {
                double gx0 = gns.coords.getEntry(0);
                double gy0 = gns.coords.getEntry(1);
                int sx0 = (int) (0.5 + (ax * gx0) + bx);
                int sy0 = (int) (0.5 + (ay * gy0) + by);

                double gx1 = gnd.coords.getEntry(0);
                double gy1 = gnd.coords.getEntry(1);
                int sx1 = (int) (0.5 + (ax * gx1) + bx);
                int sy1 = (int) (0.5 + (ay * gy1) + by);

                g2d.drawLine(sx0, sy0, sx1, sy1);
            }
        }

        // not the most efficient way to detect multi-edges
        Set<GeoNode> nodes = myGraph.wpg.vertexSet();
        for (GeoNode gns : nodes) {
            int idS = gns.getID();
            for (GeoNode gnd : nodes) {
                int idD = gnd.getID();
                if (idS <= idD) { // avoid listing edges twice
                    Set<GeoEdge> es = myGraph.wpg.getAllEdges(gns, gnd);
                    if (!es.isEmpty()) {
                        int ess = es.size();
                        if (1 < ess) {
                            System.out.printf("There are %d edges between %4d and %4d\n",
                                    ess, idS, idD);
                        }
                        if (idS == idD) {
                            if (1 == ess) {
                                System.out.printf("Cannot draw self-loop: %4d \n", idS);
                            } else if (1 < ess) {
                                System.out.printf("Cannot draw %d loops:   %4d \n", ess, idS);

                            }
                        }
                    }
                }
            }
        }

        boolean drawLabels = (nodes.size() <= maxSizeLabeled);
        if (drawLabels) {
            // set graphics for vertices
            int dotRadius = dotDiameter / 2;
            int fSize = 12; // points
            String labelFormat = "%2d";
            g2d.setColor(dotColor);
            for (GeoNode gni : nodes) {
                double gx = gni.coords.getEntry(0);
                double gy = gni.coords.getEntry(1);
                int sx = (int) (0.5 + (ax * gx) + bx);
                int sy = (int) (0.5 + (ay * gy) + by);

                // put a little box there
                g2d.setColor(dotColor);
                int xc = sx - dotRadius;
                int yc = sy - dotRadius;
                g2d.fillRect(xc, yc, dotDiameter, dotDiameter);
            }

            // set graphics for text labels
            // label it
            g2d.setColor(Color.BLACK);
            g2d.setFont(new Font(Font.MONOSPACED, Font.PLAIN, fSize));
            for (GeoNode gni : nodes) {
                double gx = gni.coords.getEntry(0);
                double gy = gni.coords.getEntry(1);
                int sx = (int) (0.5 + (ax * gx) + bx);
                int sy = (int) (0.5 + (ay * gy) + by);

                int xc = sx - dotRadius;
                int yc = sy - dotRadius;
                String label = String.format(labelFormat, gni.getID());
                g2d.drawString(label, xc, yc - dotRadius);
            }
        }

        g2d.dispose();
        return svgDoc;
    }

    private void drawEdges() {

    }

    public void outputSVGtoFile(SVGDocument svgDoc, String filePath) {
        String svgText = svgDoc.toSVGString();
        try (FileWriter fileWriter = new FileWriter(filePath)) {
            fileWriter.write(svgText);
            fileWriter.flush();
            //fileWriter.close();  // redundant
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private void drawMarginBox(double inchWidth, double inchHeight,
            double inchMargin, Color mColor,
            boolean fill, Color fColor,
            Graphics g) {
        Color c0 = g.getColor();
        int x0 = (int) (0.5 + PointsPerInch * inchMargin);
        int y0 = (int) (0.5 + PointsPerInch * inchMargin);
        int x1 = (int) (0.5 + PointsPerInch * (inchWidth - inchMargin));
        int y1 = (int) (0.5 + PointsPerInch * (inchHeight - inchMargin));

        // Create a copy of the Graphics instance
        Graphics2D g2d = (Graphics2D) g.create();

        // set new pen characteristics
        Stroke solidNarrow = new BasicStroke(1, // width, perhaps in points?
                BasicStroke.CAP_BUTT, // end caps
                BasicStroke.JOIN_BEVEL, // line joins
                0
        );
        g2d.setStroke(solidNarrow);

        if (fill) {
            g.setColor(fColor);
            int[] xPoints = {x0, x0, x1, x1, x0};
            int[] yPoints = {y0, y1, y1, y0, y0};
            g.fillPolygon(xPoints, yPoints, xPoints.length);
        }
        g2d.setColor(mColor);
        g2d.drawLine(x0, y0, x0, y1); // down across left side
        g2d.drawLine(x0, y1, x1, y1); // right across bottom edge
        g2d.drawLine(x1, y1, x1, y0); // up across right edge
        g2d.drawLine(x1, y0, x0, y0); // left across top edge
        g2d.dispose();

        // restore previous pen characteristics
        g.setColor(c0);

    }

    /**
     * Return Tuple4 of graph bounds
     *
     * @return Tuple4<Double, Double, Double, Double> of minX, minY, maxX, maxY
     */
    private Tuple4<Double, Double, Double, Double> graphBounds() {
        double minX = Double.POSITIVE_INFINITY;
        double maxX = Double.NEGATIVE_INFINITY;
        double minY = Double.POSITIVE_INFINITY;
        double maxY = Double.NEGATIVE_INFINITY;
        Set<GeoGraph.GeoNode> nodes = myGraph.wpg.vertexSet();
        for (GeoGraph.GeoNode gni : nodes) {
            minX = Math.min(gni.coords.getEntry(0), minX);
            minY = Math.min(gni.coords.getEntry(1), minY);
            maxX = Math.max(maxX, gni.coords.getEntry(0));
            maxY = Math.max(maxY, gni.coords.getEntry(1));
        }
        return new Tuple4<>(minX, minY, maxX, maxY);
    }

}

// =============================================================================
