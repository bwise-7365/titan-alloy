/*
 *  ---------------------------------------------------
 *         Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 * 
 */
package groupw.DCVRP;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import static groupw.DCVRP.VRController.TheVRC;

/**
 *
 * @author BenWise
 */
public class Backlog implements CountedItem {

    public Backlog(String tn) {
        this.transportName = tn;
        reservations = new ArrayList<>();
        this.idNum = ItemCounter.makeID();
    }

    static public class Reservation {
        public String transportName;
        public String serialName;
        public String destNodeName;
        public String midNodeName; // the node to which this transport will take this serial
        public double area;
        public double weight;
        public double roundTripTime; // to-from range over transport cruise speed

        public Reservation(String transportName, String serialName, String destNodeName, String midNodeName, double area, double weight, double roundTripTime) {
            this.transportName = transportName;
            this.serialName = serialName;
            this.destNodeName = destNodeName;
            this.midNodeName = midNodeName;
            this.area = area;
            this.weight = weight;
            this.roundTripTime = roundTripTime;
        }
    }

    public List<Reservation> reservations = null;
    public String transportName;

    /**
     * Append this reservation to the back of the reservations-list
     *
     * @param r the reservation to be appended
     * @return length of updated reservations-list
     */
    public int appendReservation(Reservation r) {
        reservations.add(r);
        return reservations.size();
    }

    /**
     * Return the place-in-queue of this reservation.
     * <p>
     * Returns negative if not found
     *
     * @param serialName the serial whose reservation is to be found
     * @return position in queue (zero is first); negative if not present.
     */
    public int reservationIndex(String serialName) {
        int ndx = -1;
        for (int i = 0; (ndx < 0) && (i < reservations.size()); i++) {
            if (serialName.equals(reservations.get(i).serialName)) {
                ndx = i;
            }
        }
        return ndx;
    }

    /**
     * Remove the reservation(s?) with this serial name from the reservations-list.
     *
     * @param serialName serial whose reservation is to be removed
     * @return length of updated reservations-list
     */
    public int removeReservation(String serialName) {
        List<Reservation> rl2 = new ArrayList<>();
        for (Reservation r : reservations) {
            if (!serialName.equals(r.serialName)) {
                rl2.add(r);
            } else { // record that the serial is no longer in a backlog
                Serial s = TheVRC.getSerialMap().get(serialName);
                s.currBacklog = null;
            }
        }
        reservations = rl2;
        return reservations.size();
    }

    /**
     * Clear out all existing reservations and mark serials as no longer backlogged.
     * <p>
     * This is useful if the associated vehicle is destroyed. Notice that items
     * in the backlog but not
     */
    public void clear() {
        Map<String, Serial> serialMap = TheVRC.getSerialMap();
        for (Reservation r : reservations) {
            Serial s = serialMap.get(r.serialName);
            if (null != s) {
                s.currBacklog = null;
            }
        }
        reservations = new ArrayList<>();
    }

    /**
     * Estimate the average round-trip time for everything at or before the n-th reservation.
     * Notice that the average for 0-th reservation is average over one round-trip time.
     *
     * @param n starting at zero, last reservation to include
     * @return
     */
    public double averageRTT(int n) {
        double avrg = 0.0;
        if ((null == reservations) || (reservations.size() < n)) {
            final String msg = "Backlog of reservations smaller than requested average: " + reservations.size() + " vs " + n;
            throw new RuntimeException(msg);
        }
        for (int i = 0; i <= n; i++) {
            avrg = avrg + reservations.get(i).roundTripTime;
        }
        avrg = avrg / (n + 1.0);
        return avrg;
    }

    /**
     * Total cargo area for everything at or before the n-th reservation.
     *
     * @param n starting at zero, last reservation to include
     * @return total area
     */
    public double totalArea(int n) {
        double a = 0.0;
        if ((null == reservations) || (reservations.size() < n)) {
            final String msg = "Backlog of reservations smaller than requested total: " + reservations.size() + " vs " + n;
            throw new RuntimeException(msg);
        }
        for (int i = 0; i <= n; i++) {
            a = a + reservations.get(i).area;
        }
        return a;
    }

    /**
     * Total cargo weight for everything at or before the n-th reservation.
     *
     * @param n starting at zero, last reservation to include
     * @return total weight
     */
    public double totalWeight(int n) {
        double w = 0.0;
        if ((null == reservations) || (reservations.size() < n)) {
            final String msg = "Backlog of reservations smaller than requested total: " + reservations.size() + " vs " + n;
            throw new RuntimeException(msg);
        }
        for (int i = 0; i <= n; i++) {
            w = w + reservations.get(i).weight;
        }
        return w;
    }

    public double arrivalTime(String vName, int n) {
        if ((null == reservations) || (reservations.size() < n)) {
            final String msg = "Backlog of reservations smaller than requested total: " + reservations.size() + " vs " + n;
            throw new RuntimeException(msg);
        }
        int num = tripNumber(vName, n);
        double rtMean = averageRTT(n);

        double hypoArriveTime = (num - 0.5) * rtMean;
        return hypoArriveTime;
    }

    public int tripNumber(String vName, int n) {
        if ((null == reservations) || (reservations.size() < n)) {
            final String msg = "Backlog of reservations smaller than requested total: " + reservations.size() + " vs " + n;
            throw new RuntimeException(msg);
        }
        ReadTransportVehicleCSV.DataField vRec = TheVRC.getVehicleDataMap().get(vName);
        ReadTransportTypeCSV.DataField vtRec = TheVRC.getVehicleTypeMap().get(vRec.type);

        double aTotal = totalArea(n);
        double aTrans = vtRec.cargoArea;
        int numA = (int) Math.ceil(aTotal / aTrans);

        double wTotal = totalWeight(n);
        double wTrans = vtRec.cargoWeight;
        int numW = (int) Math.ceil(wTotal / wTrans);

        int num = (numA > numW) ? numA : numW;
        return num;
    }

    public int numTrips() {
        int nt = 0;
        if ((null != reservations) && (0 < reservations.size())) {
            nt = tripNumber(transportName, reservations.size() - 1);
        }
        return nt;
    }

    /**
     * Average round trip time for everything including a hypothetical reservation.
     *
     * @param rttSerial estimated round trip time for a serial that might join backlog
     * @return average time
     */
    public double hypoAverageRTT(double rttSerial) {
        double rtSum = rttSerial;
        int count = 1;
        if (null != reservations) {
            for (int i = 0; i < reservations.size(); i++) {
                rtSum = rtSum + reservations.get(i).roundTripTime;
                count = count + 1;
            }
        }
        return (rtSum / count);
    }

    /**
     * Total cargo area for everything including a hypothetical reservation.
     *
     * @param aSerial area of serial that might join backlog
     * @return total area
     */
    public double hypoArea(double aSerial) {
        double a = aSerial;
        if (null != reservations) {
            for (int i = 0; i < reservations.size(); i++) {
                a = a + reservations.get(i).area;
            }
        }
        return a;
    }

    /**
     * Total cargo weight for everything including a hypothetical reservation.
     *
     * @param wSerial weight of serial that might join backlog
     * @return total weight
     */
    public double hypoWeight(double wSerial) {
        double w = wSerial;
        if (null != reservations) {
            for (int i = 0; i < reservations.size(); i++) {
                w = w + reservations.get(i).weight;
            }
        }
        return w;
    }

    /**
     * How long until this is delivered, if it joins the back of the Backlog
     *
     * @param vName     name of aSerial particular vehicle
     * @param aSerial   area of this potential serial
     * @param wSerial   weight of this potential serial
     * @param rttSerial round trip time of this potential serial
     * @return
     */
    public double hypoArrivalTime(String vName, double aSerial, double wSerial, double rttSerial) {
        int num = hypoTripNumber(vName, aSerial, wSerial);
        double rtMean = hypoAverageRTT(rttSerial);

        double hypoArriveTime = (num - 0.5) * rtMean;
        return hypoArriveTime;
    }

    /**
     * If serial with this area and weight join the back of the backlog, which trip would it get
     *
     * @param vName   name of aSerial particular vehicle
     * @param aSerial area of this potential serial
     * @param wSerial weight of this potential serial
     * @return expected trip number, starting at 1
     */
    public int hypoTripNumber(String vName, double aSerial, double wSerial) {
        ReadTransportVehicleCSV.DataField vRec = TheVRC.getVehicleDataMap().get(vName);
        ReadTransportTypeCSV.DataField vtRec = TheVRC.getVehicleTypeMap().get(vRec.type);

        double aTotal = hypoArea(aSerial);
        double aTrans = vtRec.cargoArea;
        int numA = (int) Math.ceil(aTotal / aTrans);

        double wTotal = hypoWeight(wSerial);
        double wTrans = vtRec.cargoWeight;
        int numW = (int) Math.ceil(wTotal / wTrans);

        int num = (numA > numW) ? numA : numW;
        return num;
    }

    public int numReservations() {
        return reservations.size();
    }

    /**
     * The numerical ID is mostly for identifying almost-anonymous
     * data structures during debugging. See the comments on CountedItem class.
     */
    @Override
    public long getID() {
        return idNum;
    }

    private final long idNum;

}

// =============================================================================
