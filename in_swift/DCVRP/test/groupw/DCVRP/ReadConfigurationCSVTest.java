// Copyright Group W, SPA. All Rights Reserved.
package groupw.DCVRP;

import java.io.File;
import java.util.List;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

/**
 * @author JoshuaSteakelum
 */
public class ReadConfigurationCSVTest {

    public ReadConfigurationCSVTest() {
    }
    
    @Test
    public void testReadConfiguration(){
        String currentDir = System.getProperty("user.dir") + File.separator + "test";
        String ifPath = currentDir + File.separator + "Data";
        System.out.printf("Current directory: >%s< \n", currentDir);
        
        String fileName = "configuration.csv";
        
        List<ReadConfigurationCSV.DataField> res = ReadConfigurationCSV.readCSV(ifPath, fileName);

        // verify that eight rows were supplied
        assertEquals(ReadConfigurationCSV.DataField.numRecords,  res.size());

        // verify that none of the required types were missing
        assert (null == ReadConfigurationCSV.getMissingFields(res));

        // verify that the required files exist
        List<String> fields = ReadConfigurationCSV.FileTypes;
        for(String t : fields) {
            boolean exists = false;
            for (ReadConfigurationCSV.DataField df : res) {
                if (df.fName.equals(t)) {
                    exists = true;
                    System.out.println(String.format("%s: %s", df.fName, df.fPath));
                    break;
                }
            }
            assert (exists);
        }
    }

}
// =============================================================================
