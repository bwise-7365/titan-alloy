/*
 *  ---------------------------------------------------
 *         Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 * 
 */

package groupw.DCVRP;

import groupw.Network.NWUtils;
import java.awt.Component;
import java.io.File;
import java.util.*;
import javax.swing.JFileChooser;
import javax.swing.filechooser.FileNameExtensionFilter;

/**
 * Utilities for De-Centralized Vehicle Routing problems
 * @author BenWise
 */
public class UtilsDCVR {
    /**
     * Names (unit, Serial, domain, transport) have a minimum length, e.g. two characters
     */
    public static final int MinimumNameLength = 2;

    public static final char csvCommentChar = '#';
    
    public static NWUtils.Tuple2<String, String> chooseCsvFile(String title, Component parent) {
        JFileChooser fileChooser = new JFileChooser();
        fileChooser.setCurrentDirectory(new File(System.getProperty("user.dir"))); // start somewhere close to test data
        fileChooser.setDialogTitle(title);

        FileNameExtensionFilter csvFilter = new FileNameExtensionFilter("CSV Files", "csv");
        fileChooser.setFileFilter(csvFilter);
        
        NWUtils.Tuple2<String, String> filePair = null;

        int result = fileChooser.showOpenDialog(parent);

        if (result == JFileChooser.APPROVE_OPTION) {
            File selectedFile = fileChooser.getSelectedFile();
            String filePath = selectedFile.getParent();
            System.out.printf("filePath: %s \n", filePath);
            String fileName = selectedFile.getName();
            System.out.printf("fileName: %s \n", fileName);
            filePair = new NWUtils.Tuple2<>(filePath, fileName);
        }
        return filePair;
    }


    static public Set<String> buildPortDomains(
            String portName,
            List<ReadPortAccessCSV.DataField> portAccess, // by vehicle type
            List<ReadTransportDomainCSV.DataField> transDomain // including domain
    ) {

        // find the Set of transports that can access this location
        Set<String> portTrans = new HashSet<>(1);
        for (ReadPortAccessCSV.DataField paRed : portAccess) {
            if (paRed.portName.equals(portName)) {
                portTrans.add(paRed.transType);
            }
        }

        // find the Set of domains those transports can cross
        Set<String> domains = new HashSet<>(1);
        for (String tType : portTrans) {
            for (ReadTransportDomainCSV.DataField tmpTType : transDomain) {
                if (tType.equals(tmpTType.type)) {
                    domains.add(tmpTType.domain);
                }
            }
        }
        return domains;
    }

    static public Map<String, Set<String>> buildPortDomainMap (
            List<ReadPortAccessCSV.DataField> portAccess, // by vehicle type
            List<ReadTransportDomainCSV.DataField> transDomains) // including domain)
    {
        Set<String> portNames = new HashSet<>(2);
        for (ReadPortAccessCSV.DataField paRec : portAccess) {
            portNames.add(paRec.portName);
        }
        Map<String, Set<String>> pdMap = new HashMap<>(2);
        for (String pName : portNames) {
            Set<String> pDomains = buildPortDomains(pName, portAccess, transDomains);
            pdMap.put(pName, pDomains);
        }
        return pdMap;
    }

    /**
     * Given the names of two ports, find the set of domains used by transports
     * which can access both.
     *
     * @param srcPN
     * @param tgtPN
     * @param portAccess
     * @param transDomains
     * @return
     */
    static public Set<String> buildArcDomains(
            String srcPN, String tgtPN,
            List<ReadPortAccessCSV.DataField> portAccess, // by vehicle type
            List<ReadTransportDomainCSV.DataField> transDomains // including domain
    ) {
        Set<String> srcDomains = buildPortDomains(srcPN, portAccess, transDomains);
        Set<String> tgtDomains = buildPortDomains(tgtPN, portAccess, transDomains);
        srcDomains.retainAll(tgtDomains); // find intersection
        return srcDomains;
    }
}


// =============================================================================