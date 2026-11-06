package aji.score;

import aji.model.Params;
import aji.model.SystemModel;

import java.util.List;

/**
 * The outcome of scoring one system.
 *
 * @param model           the scored system (layers as given, not ranked)
 * @param params          the switches used
 * @param declaredLayers  for each rank 1..n (index rank - 1), the layer number given in the model
 * @param modules         every module's terms, in layer order
 * @param edges           the dependencies by class
 * @param mScore          the M-score, a fraction in [0, 1]
 * @param propagationCost propagation cost, a fraction in (0, 1]
 */
public record Result(SystemModel model, Params params, List<Integer> declaredLayers,
                     List<ModuleScore> modules, EdgeClasses edges, double mScore,
                     double propagationCost) {

    public Result {
        declaredLayers = List.copyOf(declaredLayers);
        modules = List.copyOf(modules);
    }

    /** n, the number of non-empty layers. */
    public int layerCount() {
        return declaredLayers.size();
    }

    public double sizeLoss() {
        return modules.stream().mapToDouble(ModuleScore::sizeLoss).sum();
    }

    public double crossLoss() {
        return modules.stream().mapToDouble(ModuleScore::crossLoss).sum();
    }

    public double innerLoss() {
        return modules.stream().mapToDouble(ModuleScore::innerLoss).sum();
    }
}
