/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.SimpleIADS;

/**
 *
 * @author BenWise
 */
public class rpsls {

    static public enum Move {
        Rock, Paper, Scissor, Lizard, Spock
    }

    static public int defeats(Move blue, Move red) {
        int r = 0; // tie
        if (blue != red) {
            switch (blue) {
                case Rock:
                    if ((red == Move.Scissor) || (red == Move.Lizard)) {
                        r = +1;
                    } else {
                        r = -1;
                    }
                    break;

                case Paper:
                    if ((red == Move.Rock) || (red == Move.Spock)) { //  (red == Move.Rock)
                        r = +1;
                    } else {
                        r = -1;
                    }
                    break;

                case Scissor:
                    if ((red == Move.Paper) || (red == Move.Lizard)) {
                        r = +1;
                    } else {
                        r = -1;
                    }
                    break;

                case Lizard:
                    if ((red == Move.Paper) || (red == Move.Spock)) {
                        r = +1;
                    } else {
                        r = -1;
                    }
                    break;

                case Spock:
                    if ((red == Move.Scissor) || (red == Move.Rock)) { //  ((red == Move.Scissor) || (red == Move.Rock) || (red == Move.Paper))
                        r = +1;
                    } else {
                        r = -1;
                    }
                    break;
            }
        }
        return r;
    }
}

// =============================================================================
