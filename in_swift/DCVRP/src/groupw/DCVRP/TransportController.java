// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

/**
 * This object acts as the 'brain' to coordinate multiple transport vehicles
 * and multiple serials. As outlined in the
 * "Decentralized Architecture for the Vehicle Routing Problem"
 * paper, it will eventually do two-sided matching.
 * Transports will develop offers to move serials and
 * Serials will prioritize and accept offers.
 * E.G., it keeps track of which Serials are already scheduled
 * to avoid double-booking.
 * The transport object actually moves Serials from Node to Node.
 * @author BenWise
 */
public class TransportController {

    public VRController dc;


}


// =============================================================================