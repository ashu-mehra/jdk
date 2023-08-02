/*
 * Copyright (c) 2020, 2023, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

import java.io.File;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import jdk.test.lib.Utils;
import jdk.test.lib.apps.LingeredApp;
import jdk.test.lib.JDKToolLauncher;
import jdk.test.lib.process.OutputAnalyzer;
import jdk.test.lib.SA.SATestUtils;
import jtreg.SkippedException;

/**
 * @test
 * @bug 8240990
 * @summary Test clhsdb dumpclass command
 * @requires vm.hasSA
 * @library /test/lib
 * @run driver ClhsdbDumpclass
 */

public class ClhsdbDumpclass {
    static final String APP_DOT_CLASSNAME = LingeredApp.class.getName();
    static final String APP_SLASH_CLASSNAME = APP_DOT_CLASSNAME.replace('.', '/');

    public static void main(String[] args) throws Exception {
        if (SATestUtils.needsPrivileges()) {
            // This test will create a file as root that cannot be easily deleted, so don't run it.
            throw new SkippedException("Cannot run this test on OSX if adding privileges is required.");
        }
        System.out.println("Starting ClhsdbDumpclass test");

        LingeredAppForClassDump theApp = new LingeredAppForClassDump();
        try {
            LingeredApp.startApp(theApp);
            System.out.println("Started LingeredAppForClassDump with pid " + theApp.getPid());

	    testLongDouble(theApp);
	    testNestMates(theApp);
	    testStackMapTable(theApp);
        } catch (SkippedException se) {
            throw se;
        } catch (Exception ex) {
            throw new RuntimeException("Test ERROR " + ex, ex);
        } finally {
            LingeredApp.stopApp(theApp);
        }
    }

    private static void dumpClasses(long appPid, List<String> classlist) throws Exception {
	ClhsdbLauncher test = new ClhsdbLauncher();

	for (String cls: classlist) {
	    String cmd = "dumpclass " + cls;
	    List<String> cmds = List.of(cmd);
	    Map<String, List<String>> unExpStrMap = new HashMap<>();
	    unExpStrMap.put(cmd, List.of("class not found"));
	    test.run(appPid, cmds, null, unExpStrMap);
	    File classFile = new File(cls + ".class");
	    if (!classFile.exists()) {
		throw new RuntimeException("FAILED: Cannot find dumped .class file");
	    }
	}
    }

    private static OutputAnalyzer runJavapOn(String cls) throws Exception {
	// Run javap on the generated class file to make sure it's valid.
	JDKToolLauncher launcher = JDKToolLauncher.createUsingTestJDK("javap");
	launcher.addVMArgs(Utils.getTestJavaOpts());
	launcher.addToolArg("-v");
	launcher.addToolArg("-c");
	launcher.addToolArg("-p");
	launcher.addToolArg(cls + ".class");
	System.out.println("> javap " + cls + ".class");
	List<String> cmdStringList = Arrays.asList(launcher.getCommand());
	ProcessBuilder pb = new ProcessBuilder(cmdStringList);
	Process javap = pb.start();
	OutputAnalyzer out = new OutputAnalyzer(javap);
	javap.waitFor();
	// do basic test here
	out.shouldHaveExitValue(0);
	out.shouldMatch("class " + cls.replaceAll("\\$", "\\\\\\$"));
	return out;
    }

    public static void testLongDouble(LingeredApp app) throws Exception {
        String classToDump = "ClassWithLongDouble";
        var classlist = List.of(classToDump);

	dumpClasses(app.getPid(), classlist);

	// Run javap on the generated class file to make sure it's valid
	// and do some sanity checks on the output
	OutputAnalyzer out = runJavapOn(classToDump);
	out.shouldContain("ConstantValue: long " + ClassWithLongDouble.LONG_VALUE + "l");
	out.shouldContain("ConstantValue: double " + ClassWithLongDouble.DOUBLE_VALUE + "d");
    }

    public static void testNestMates(LingeredApp app) throws Exception {
        String noNestMate = "NoNestMate";
        String nestLvl1 = "NestLvl1";
        String nestLvl2 = nestLvl1 + "$NestLvl2";
        String nestLvl3 = nestLvl2 + "$NestLvl3";
        var classlist = List.of(noNestMate, nestLvl1, nestLvl2, nestLvl3);

        dumpClasses(app.getPid(), classlist);

	// Run javap on the generated class file to make sure it's valid
	// and do some sanity checks on the output
	OutputAnalyzer out = runJavapOn(noNestMate);
	out.shouldNotContain("NestMembers");
	out.shouldNotContain("NestHost");

	out = runJavapOn(nestLvl1);
	// Match following pattern:
	// NestMembers:
	//     NestLvl2
	//     NestLvl2$NestLvl3
	out.shouldMatch("NestMembers:[\\s]+" + nestLvl2.replaceAll("\\$", "\\\\\\$") + "[\\s]+" + nestLvl3.replaceAll("\\$", "\\\\\\$"));

	out = runJavapOn(nestLvl2);
	out.shouldNotContain("NestMembers");
	out.shouldContain("NestHost: class " + nestLvl1);

	out = runJavapOn(nestLvl3);
	out.shouldNotContain("NestMembers");
	out.shouldContain("NestHost: class " + nestLvl1);
    }

    public static void testStackMapTable(LingeredApp app) throws Exception {
        String classToDump = app.getClass().getName();
        var classlist = List.of(classToDump);

	dumpClasses(app.getPid(), classlist);

	// Run javap on the generated class file to make sure it's valid
	// and do some sanity checks on the output
	OutputAnalyzer out = runJavapOn(classToDump);
	out.shouldContain("StackMapTable:");
    }

}
