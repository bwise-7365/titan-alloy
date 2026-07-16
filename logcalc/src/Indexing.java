// =============================================
// Copyright Ben Paul Wise. All Rights Reserved.
// =============================================

import java.util.List;
import java.util.Objects;
import java.util.function.IntFunction;
import java.util.stream.IntStream;

// Nominal subscript types and the containers they index.
//
// Each kind of subscript is its own type, so the compiler rejects
// demand.get(assetType, node) even though both wrap a plain int.
// Capacity nodes and demand nodes are distinct types because they are
// distinct index spaces with distinct sizes; conflating them is the
// mistake this file exists to prevent.
//
// Containers take the factory for their index type and use it once, in the
// constructor, to build the full set of subscripts. keys()/rows()/cols()
// hand back that cached immutable list, so iterating allocates nothing and
// the bounds always come from the right dimension.
//
// Storage is flat and row-major: cell (i, j) lives at i * nJ + j, so
// iterating j innermost walks memory in order.

sealed interface Ix permits CapNode, DmndNode, AType, TType {
    int i();
}

record CapNode(int i) implements Ix {}

record DmndNode(int i) implements Ix {}

record AType(int i) implements Ix {}

record TType(int i) implements Ix {}

// One-dimensional double storage, subscripted by I.
final class DoubleVec<I extends Ix> {
    private final double[] cells;
    private final List<I> keys;

    private DoubleVec(int n, IntFunction<I> mk) {
        this.cells = new double[n];
        this.keys = IntStream.range(0, n).mapToObj(mk).toList();
    }

    static <I extends Ix> DoubleVec<I> of(int n, IntFunction<I> mk) {
        return new DoubleVec<>(n, mk);
    }

    double get(I i)              { return cells[idx(i)]; }
    void set(I i, double v)      { cells[idx(i)] = v; }
    void add(I i, double v)      { cells[idx(i)] += v; }
    List<I> keys()               { return keys; }
    int size()                   { return cells.length; }

    private int idx(I i) { return Objects.checkIndex(i.i(), cells.length); }
}

// Two-dimensional double storage, subscripted by I then J.
final class DoubleGrid<I extends Ix, J extends Ix> {
    private final double[] cells;
    private final int nI, nJ;
    private final List<I> rows;
    private final List<J> cols;

    private DoubleGrid(int nI, IntFunction<I> mkI, int nJ, IntFunction<J> mkJ) {
        this.nI = nI;
        this.nJ = nJ;
        this.cells = new double[nI * nJ];
        this.rows = IntStream.range(0, nI).mapToObj(mkI).toList();
        this.cols = IntStream.range(0, nJ).mapToObj(mkJ).toList();
    }

    static <I extends Ix, J extends Ix> DoubleGrid<I, J> of(
            int nI, IntFunction<I> mkI, int nJ, IntFunction<J> mkJ) {
        return new DoubleGrid<>(nI, mkI, nJ, mkJ);
    }

    double get(I i, J j)             { return cells[idx(i, j)]; }
    void set(I i, J j, double v)     { cells[idx(i, j)] = v; }
    void add(I i, J j, double v)     { cells[idx(i, j)] += v; }
    List<I> rows()                   { return rows; }
    List<J> cols()                   { return cols; }

    private int idx(I i, J j) {
        return Objects.checkIndex(i.i(), nI) * nJ + Objects.checkIndex(j.i(), nJ);
    }
}

// Two-dimensional int storage, subscripted by I then J.
final class IntGrid<I extends Ix, J extends Ix> {
    private final int[] cells;
    private final int nI, nJ;
    private final List<I> rows;
    private final List<J> cols;

    private IntGrid(int nI, IntFunction<I> mkI, int nJ, IntFunction<J> mkJ) {
        this.nI = nI;
        this.nJ = nJ;
        this.cells = new int[nI * nJ];
        this.rows = IntStream.range(0, nI).mapToObj(mkI).toList();
        this.cols = IntStream.range(0, nJ).mapToObj(mkJ).toList();
    }

    static <I extends Ix, J extends Ix> IntGrid<I, J> of(
            int nI, IntFunction<I> mkI, int nJ, IntFunction<J> mkJ) {
        return new IntGrid<>(nI, mkI, nJ, mkJ);
    }

    int get(I i, J j)                { return cells[idx(i, j)]; }
    void set(I i, J j, int v)        { cells[idx(i, j)] = v; }
    void add(I i, J j, int v)        { cells[idx(i, j)] += v; }
    List<I> rows()                   { return rows; }
    List<J> cols()                   { return cols; }

    private int idx(I i, J j) {
        return Objects.checkIndex(i.i(), nI) * nJ + Objects.checkIndex(j.i(), nJ);
    }
}

// One-dimensional reference storage, subscripted by I.
// The unchecked cast is safe: set(I, T) is the only writer, so every
// live cell holds a T or null.
final class Vec<T, I extends Ix> {
    private final Object[] cells;
    private final List<I> keys;

    private Vec(int n, IntFunction<I> mk) {
        this.cells = new Object[n];
        this.keys = IntStream.range(0, n).mapToObj(mk).toList();
    }

    static <T, I extends Ix> Vec<T, I> of(int n, IntFunction<I> mk) {
        return new Vec<>(n, mk);
    }

    @SuppressWarnings("unchecked")
    T get(I i)                   { return (T) cells[idx(i)]; }
    void set(I i, T v)           { cells[idx(i)] = v; }
    List<I> keys()               { return keys; }
    int size()                   { return cells.length; }

    private int idx(I i) { return Objects.checkIndex(i.i(), cells.length); }
}

// Two-dimensional reference storage, subscripted by I then J.
// Boxes if T is a wrapper type; prefer DoubleGrid/IntGrid for numeric cells.
final class Grid<T, I extends Ix, J extends Ix> {
    private final Object[] cells;
    private final int nI, nJ;
    private final List<I> rows;
    private final List<J> cols;

    private Grid(int nI, IntFunction<I> mkI, int nJ, IntFunction<J> mkJ) {
        this.nI = nI;
        this.nJ = nJ;
        this.cells = new Object[nI * nJ];
        this.rows = IntStream.range(0, nI).mapToObj(mkI).toList();
        this.cols = IntStream.range(0, nJ).mapToObj(mkJ).toList();
    }

    static <T, I extends Ix, J extends Ix> Grid<T, I, J> of(
            int nI, IntFunction<I> mkI, int nJ, IntFunction<J> mkJ) {
        return new Grid<>(nI, mkI, nJ, mkJ);
    }

    @SuppressWarnings("unchecked")
    T get(I i, J j)                  { return (T) cells[idx(i, j)]; }
    void set(I i, J j, T v)          { cells[idx(i, j)] = v; }
    List<I> rows()                   { return rows; }
    List<J> cols()                   { return cols; }

    private int idx(I i, J j) {
        return Objects.checkIndex(i.i(), nI) * nJ + Objects.checkIndex(j.i(), nJ);
    }
}

// =============================================
// Copyright Ben Paul Wise. All Rights Reserved.
// =============================================
