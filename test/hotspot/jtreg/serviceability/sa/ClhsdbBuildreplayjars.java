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

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
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
 * @summary Test clhsdb buildreplayjars command
 * @requires vm.hasSA
 * @library /test/lib
 * @run driver ClhsdbBuildreplayjars
 */

public class ClhsdbBuildreplayjars {
    static final String APP_DOT_CLASSNAME = LingeredApp.class.getName();
    static final String APP_SLASH_CLASSNAME = APP_DOT_CLASSNAME.replace('.', '/');

    public static void main(String[] args) throws Exception {
        if (SATestUtils.needsPrivileges()) {
            // This test will create a file as root that cannot be easily deleted, so don't run it.
            throw new SkippedException("Cannot run this test on OSX if adding privileges is required.");
        }
        System.out.println("Starting ClhsdbDumpclass test");

        LingeredApp theApp = null;
        String prefix = "sa";
        String param = "all";
        String bootJarFile = prefix + "boot" + ".jar";
        String appJarFile = prefix + "app" + ".jar";
        try {
            ClhsdbLauncher test = new ClhsdbLauncher();

            theApp = LingeredApp.startApp();
            System.out.println("Started LingeredApp with pid " + theApp.getPid());

            // Run "buildreplayjars boot"
            String cmd = "buildreplayjars" + " " + param + " " + prefix;
            List<String> cmds = List.of(cmd);
            Map<String, List<String>> unExpStrMap = new HashMap<>();
            unExpStrMap.put(cmd, List.of("class not found"));
            test.run(theApp.getPid(), cmds, null, unExpStrMap);
            Path bootJar = Paths.get(bootJarFile);
            if (!Files.exists(bootJar)) {
                throw new RuntimeException("FAILED: Expected " + bootJarFile + " to be present");
            }
            Path appJar = Paths.get(appJarFile);
            if (!Files.exists(appJar)) {
                throw new RuntimeException("FAILED: Expected " + appJarFile + " to be present");
            }
        } catch (Exception ex) {
            throw new RuntimeException("Test ERROR " + ex, ex);
        } finally {
            LingeredApp.stopApp(theApp);
        }
        try {
            // Run LingeredApp using generated jar files
            theApp = new LingeredApp();
            theApp.setUseDefaultClasspath(false);
            LingeredApp.startApp(theApp, "-Xlog:class+load", "--patch-module", "java.base=" + bootJarFile, "-cp", appJarFile);
            System.out.println("Started LingeredApp (using generated jar files) with pid " + theApp.getPid());
        } catch (Exception ex) {
            throw new RuntimeException("Test ERROR " + ex, ex);
        } finally {
            LingeredApp.stopApp(theApp);
        }
        System.out.println("Test PASSED");
    }
}
