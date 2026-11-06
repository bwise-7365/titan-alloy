package aji.score;

import aji.model.Module;

import java.util.Comparator;

/**
 * One module's terms in the M-score sum.
 *
 * @param module       the module
 * @param totalFiles   N, the number of files in the system
 * @param lowerDeps    the cross-layer numerator (pairs or files, as the parameters say)
 * @param lowerFiles   the number of files in the lower layers that count
 * @param internalDeps dependencies between files of this module
 * @param sp           size penalty factor SP
 * @param clddp        cross-layer dependency density penalty CLDDP
 * @param imcp         inner-module complexity penalty IMCP
 * @param contribution SF * (1 - CLDDP) * (1 - IMCP), where SF = |M| / N * SP
 */
public record ModuleScore(Module module, int totalFiles, int lowerDeps, int lowerFiles,
                          int internalDeps, double sp, double clddp, double imcp,
                          double contribution) {

    /** The modules that lose the most first; equal losses in the order of their first files. */
    public static final Comparator<ModuleScore> BY_LOSS =
            Comparator.comparingDouble(ModuleScore::lost).reversed()
                    .thenComparing(ms -> ms.module().files().get(0));

    /** |M| / N: what the module would contribute with no penalty at all. */
    public double weight() {
        return (double) module.size() / totalFiles;
    }

    /**
     * What each unit of {@link #lowerDeps} takes from the module's contribution: the
     * contribution falls in a straight line as the cross-layer count rises. It is 0 in the
     * bottom layer, where nothing is counted.
     */
    public double costPerLowerDep() {
        return lowerFiles == 0 ? 0 : sp * (1 - imcp) / ((double) totalFiles * lowerFiles);
    }

    /** weight - contribution: the share of the score this module loses. */
    public double lost() {
        return weight() - contribution;
    }

    /**
     * The loss split into three parts that add up to {@link #lost()}: the size penalty first,
     * then the cross-layer penalty on what remains, then the inner-module penalty.
     */
    public double sizeLoss() {
        return weight() * (1 - sp);
    }

    public double crossLoss() {
        return weight() * sp * clddp;
    }

    public double innerLoss() {
        return weight() * sp * (1 - clddp) * imcp;
    }
}
