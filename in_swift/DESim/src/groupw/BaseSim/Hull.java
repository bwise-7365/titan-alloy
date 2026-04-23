/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */

package groupw.BaseSim;


import static groupw.BaseSim.DSUtils.showVector;
import static groupw.Network.NWUtils.ReportingLevel.High;
import static groupw.Network.NWUtils.rLevelLE;
import org.apache.commons.math4.legacy.linear.RealVector;

/**
 * A Hull is simply an Entity with a location and type-name. This includes
 * mobile things like missiles as well as stationary things like warehouses. A
 * type like "Sidewinder" is likely to have only dynamics and intercept
 * guidance, toward some mobile or stationary hull. A type like "F-15 Eagle" is
 * likely to have dynamics to implement movement, a radar, a message processor,
 * and so on. Note that we 'register' each Hull with the Scheduler using the ID
 * number, so they must all be unique, i.e. use the same counter
 * 'Entity.highestID'
 *
 * @author bwise
 */
public abstract class Hull
        extends Entity
        implements LocationIntrf {

    public Hull(String tn, Scheduler s) {
        super(s); // registers entity as a CountedItem
        assert (DSUtils.MINIMUM_TYPE_NAME_LENGTH <= tn.length());
        typeName = tn;
        mySim.registerHull(this);
    }

    public void showPVDT(double dt) {
        if (rLevelLE(High, mySim.rLevel)) {
            double tNow = mySim.getCurrTime();
            RealVector pTmp = this.drCurrPos(tNow);
            System.out.printf("Hull %4d position:\n", getID());
            showVector(pTmp, " %9.3f");
            RealVector vTmp = this.drCurrVel(tNow);
            double sTmp = vTmp.getNorm();
            System.out.printf("Hull %4d velocity, %8.2f\n", getID(), sTmp);
            showVector(vTmp, " %9.3f");
            System.out.printf("Hull %4d next event after %.4f\n", getID(), dt);
        }
    }

    public String getTypeName() {
        return typeName;
    }

    final protected String typeName;
    public double dtMax = 10.0; // maximum time between my EntEvent, seconds
    public PFSM myFSM = null; // the behavior to 'step'

}

// =============================================================================
