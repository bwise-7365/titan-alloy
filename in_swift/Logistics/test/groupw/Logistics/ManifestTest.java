/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */

package groupw.Logistics;

import org.junit.Test;
import org.junit.Before;
import org.junit.Assert;

/**
 *
 * @author JoshuaSteakelum
 */
public class ManifestTest {
    
    private final int uniqueCount = 50;
    private final double tolerance = 0.001;
    
    Manifest TestManifest;
    
    public ManifestTest(){
    }
    
    @Before
    public void setup() throws Exception{
        this.TestManifest = new Manifest();
    }
    
    // VALIDATE NO REPEATS ON OBJECTS LIST
    // TEST INITIAL VALUE OF 0 WORKS BUT DOESNT OVERWRITE REAL VALUE
    
    @Test
    public void TestAddInventory(){
        
        // add a bunch of items with known quantity
        for(int i = 0; i < uniqueCount; i++){
            this.TestManifest.addInventory("item" + i, 1 + i*1.0);
        }
        for(int i = 0; i < uniqueCount; i++){
            Assert.assertEquals(1 + i * 1.0, this.TestManifest.getAvailable("item" + i), tolerance);
        }
        
        // add some more to test adding capability
        // test by adding 2 to the first half of items (3 total) leaving rest alone
        for(int i = 0; i < uniqueCount / 2; i++){
            this.TestManifest.addInventory("item" + i, i*2.0);
        }

        for(int i = 0; i < uniqueCount; i++){
            if(i < (uniqueCount / 2)){
                Assert.assertEquals(1 + i * 3.0, this.TestManifest.getAvailable("item" + i), tolerance);
            }else{
                Assert.assertEquals(1 + i * 1.0, this.TestManifest.getAvailable("item" + i), tolerance);
            }
        }
    }
    
    @Test
    public void TestSubtractInventory(){
        //subtract something not there
        Assert.assertThrows(ArithmeticException.class,
                () -> {this.TestManifest.subtractInventory("notThere", 1.0);});
        //subtract something there
        this.TestManifest.addInventory("isThere", 5.0);
        //subtract more than whats there
        Assert.assertThrows(ArithmeticException.class,
                () -> {this.TestManifest.subtractInventory("isThere", 6.0);});
        //subtract less show that it still has some
        Assert.assertEquals(5.0, this.TestManifest.getAvailable("isThere"), tolerance);
        this.TestManifest.subtractInventory("isThere", 3.0);
        Assert.assertEquals(2.0, this.TestManifest.getAvailable("isThere"), tolerance);
        this.TestManifest.subtractInventory("isThere", 2.0);
    }
    
    
    /*
    @Test
    public void TestInitialInventory(){    
    }
    */
    
    @Test
    public void TestOveruseInventory(){
        this.TestManifest.addInventory("isThere", 100.0);
        Assert.assertEquals(100.0, this.TestManifest.getAvailable("isThere"), tolerance);
        this.TestManifest.useDesired("isThere", 10.0);
        Assert.assertEquals(90.0, this.TestManifest.getAvailable("isThere"), tolerance);
        this.TestManifest.useDesired("isThere", 60.0);
        Assert.assertEquals(30.0, this.TestManifest.getAvailable("isThere"), tolerance);
        this.TestManifest.useDesired("isThere", 500.0);
        Assert.assertEquals(0.0, this.TestManifest.getAvailable("isThere"), tolerance);
        
    }
    
    @Test
    public void TestContents(){
        // add unique items and verify the list length
        for(int i = 0; i < this.uniqueCount; i++){
            this.TestManifest.addInventory("item" + i, 1 + i*3.5);
        }
        Assert.assertEquals(this.uniqueCount, this.TestManifest.getItemNames().size());
        
        // add more of the same items and verify same length
        for(int i = 0; i < this.uniqueCount; i++){
            this.TestManifest.addInventory("item" + i, 1 + i*3.5);
        }
        Assert.assertEquals(this.uniqueCount, this.TestManifest.getItemNames().size());
        
        // remove 1 item fully and verify the list is 1 less
        System.out.println(this.TestManifest);
        this.TestManifest.useDesired("item0", 1000.0);
        System.out.println(this.TestManifest);
        Assert.assertEquals(this.uniqueCount - 1, this.TestManifest.getItemNames().size());
    }
    
    @Test
    public void TestOverDesired(){
        this.TestManifest.addInventory("isThere", 100.0);
        this.TestManifest.useDesired("isThere", 1000.0);
        Assert.assertEquals(0.0, this.TestManifest.getAvailable("isThere"), tolerance);
        
        this.TestManifest.useDesired("isThere", 10.0);
        this.TestManifest.useDesired("isThere", 0.0);
    }
    
    @Test
    public void TestClear(){
        
    }
    
    @Test
    public void TestUniqueCount(){
        
    }
    
}
