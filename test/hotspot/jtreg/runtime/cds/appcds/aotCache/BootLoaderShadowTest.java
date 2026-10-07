/*
 * @test
 * @summary Test the effect of shadowing between bootloader and system loader.
 * @requires vm.cds.supports.aot.class.linking
 * @library /test/lib /test/hotspot/jtreg/runtime/cds/appcds/aotCache/test-classes
 * @build BootLoaderShadowTest ClassDefiner Shadower Foo
 * @run driver jdk.test.lib.helpers.ClassFileInstaller -jar app.jar BootLoaderShadowApp
 * @run driver jdk.test.lib.helpers.ClassFileInstaller -jar shadowlib.jar ClassDefiner Shadower
 * @run driver jdk.test.lib.helpers.ClassFileInstaller -jar foo.jar Foo
 * @run driver BootLoaderShadowTest AOT
 */

import java.io.InputStream;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.function.Consumer;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

import jdk.test.lib.cds.CDSAppTester;
import jdk.test.lib.helpers.ClassFileInstaller;

public class BootLoaderShadowTest {
    private static final String mainClass = "BootLoaderShadowApp";

    public static void main(String... args) throws Exception {
        Tester tester = new Tester();
        tester.runAOTWorkflow();
    }

    static class Tester extends CDSAppTester {
        public Tester() {
            super(mainClass);
        }

        @Override
        public String[] vmArgs(RunMode runMode) {
            return new String[] { "-Xbootclasspath/a:shadowlib.jar" };
        }

        @Override
        public String classpath(RunMode runMode) {
            return "app.jar:foo.jar";
        }

        @Override
        public String[] appCommandLine(RunMode runMode) {
            boolean doShadow = runMode == RunMode.PRODUCTION;
            return new String[] {
                mainClass,
                doShadow ? "shadow" : ""
            };
        }
    }
}

class BootLoaderShadowApp {
    public static void main(String args[]) throws Exception {
        boolean doShadow = args.length >= 1 && args[0].equals("shadow");
        if (doShadow) {
            Class<?> cls = Class.forName("ClassDefiner", true, null);
            Consumer<byte[]> definer = (Consumer<byte[]>)cls.newInstance();
        
            byte[] classBytes = readJarEntry("foo.jar", "Foo.class");
            definer.accept(classBytes);
        }
        Class<?> fooCls = Class.forName("Foo");
        ClassLoader expected;
        if (doShadow) {
            expected = null;
        } else {
            expected = ClassLoader.getSystemClassLoader();
        }
        if (fooCls.getClassLoader() != expected) {
            throw new RuntimeException("Foo's classloader is " + fooCls.getClassLoader().getName() + ", but expected is " + expected);
        }
    }

    static byte[] readJarEntry(String jarFile, String entryName) throws IOException {
        try (JarFile jar = new JarFile(jarFile)) {
            JarEntry entry = jar.getJarEntry(entryName);
            if (entry == null) {
                throw new IOException("Entry not found: " + entryName);
            }

            try (var in = jar.getInputStream(entry)) {
                return in.readAllBytes();
            }
        }
    }
}
