package jdk.internal.staticanalysis;

import java.util.Collections;
import java.util.Arrays;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;

class StaticAnalyzer {
    final Set<Long> workQueue = ConcurrentHashMap.newKeySet();

    public boolean addToQueue(long[] methodHandles) {
        long[] handlesToAdd = Arrays.stream(methodHandles).filter(handle -> !workQueue.add(handle)).toArray();
        return analyze(handlesToAdd);
    }

    public native boolean analyze(long[] methodHandles);

    public static native void registerNatives();
}
