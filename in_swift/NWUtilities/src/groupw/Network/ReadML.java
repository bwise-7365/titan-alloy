package groupw.Network;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import groupw.Network.NWUtils.Tuple2;

public class ReadML {

    static public Tuple2<List<Double>, List<Double>>  readCSV(String currentDir, String ifName, String ofName) {
        boolean success = true;
        int numPoints = 0;
        int InitCap = 4000;

        List<Double> xs = new ArrayList<>(InitCap);
        List<Double> ys = new ArrayList<>(InitCap);

        List<Double> x2s = null;
        List<Double> y2s = null;
        try (BufferedReader in = new BufferedReader(new FileReader(currentDir + ifName))) {
            String str;
            while ((str = in.readLine()) != null) {
                String[] fields = str.split(",");
                int lineNum = Integer.parseInt(fields[0]);
                int xCoord = Integer.parseInt(fields[1]);
                int yCoord = Integer.parseInt(fields[2]);
                xs.add((double) xCoord);
                ys.add((double) yCoord);
                numPoints++;
                //System.out.printf("%6d:  %6d , %6d \n", lineNum, xCoord, yCoord);
            }
        } catch (IOException e) {
            System.out.println("File Read Error");
            success = false;
        }
        System.out.printf("Array length: %d vs %d \n", xs.size(), numPoints);

        if (success) {
            x2s = new ArrayList<>(InitCap);
            y2s = new ArrayList<>(InitCap);
            for (int i = 0; i < numPoints; i++) {
                if (0 == (i % 25)) { // keep only 4%
                    x2s.add(xs.get(i));
                    y2s.add(ys.get(i));
                }
            }
            System.out.printf("Reduced array length: %d vs %d \n", x2s.size(), numPoints);

        }
        Tuple2<List<Double>, List<Double>> rslt = new Tuple2<>(x2s, y2s);
        return rslt;
    }
}

// =============================================================================
