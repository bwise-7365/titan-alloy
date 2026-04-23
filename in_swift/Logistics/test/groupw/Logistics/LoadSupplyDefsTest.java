/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.Logistics;

import java.io.File;
import java.io.IOException;

import org.junit.Test;
import static org.junit.Assert.*;

/**
 * Test the class that loads the definition file for supplies
 * @author BenWise
 */
public class LoadSupplyDefsTest {

    public LoadSupplyDefsTest() {
    }

    @Test
    public void testGetInstanceFound() {

        final String directoryPath = System.getProperty("user.dir");
        System.out.println("Testing current dir = " + directoryPath);

        System.out.println("It holds these dir/file:");
        System.out.println("-----");
        // Using File class create an object for specific directory
        File directory = new File(directoryPath);

        // Using listFiles method we get all the files of a directory
        // return type of listFiles is array
        File[] files = directory.listFiles();

        // Print name of the all files present in that path
        if (files != null) {
            for (File file : files) {
                System.out.printf("  %s\n", file.getName());
            }
        }
        System.out.println("-----");

        String filePath = "example-supplies.csv";
        LoadSupplyDefs suppData = null;
        try {
            suppData = new LoadSupplyDefs(filePath);
        } catch (IOException e) {
            // we should never catch an error
            assertTrue(null != suppData);
        }
        assertTrue(null != suppData);
    }

    @Test
    public void testGetInstanceMissed() {

        String filePath = "example-supplies-NONEXISTENT.csv";
        LoadSupplyDefs suppData = null;
        try {
            suppData = new LoadSupplyDefs(filePath);
        } catch (IOException e) {
            // we should always catch an error
            assertTrue(null == suppData);
        }
        assertTrue(null == suppData);
    }

}


// =============================================================================
