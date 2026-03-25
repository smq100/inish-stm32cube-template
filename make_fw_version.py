
import os
import sys
from datetime import datetime, timezone

version_path = "./Application/inc"
version_name = "version.h"

version_major_tag = "FW_REV_MAJOR"
version_minor_tag = "FW_REV_MINOR"
version_build_tag = "FW_REV_BUILD"
version_u32_tag = "FW_REV_U32"
version_str_tag = "FW_REV_STR"

date_str_tag = "FW_DATE_STR"
build_str_tag = "FW_ISO_STR"

class FwVersion:
    VER_MAJOR_DEF = 1
    VER_MINOR_DEF = 1
    VER_BUILD_DEF = 0

    def __init__(self):
        self.major = self.VER_MAJOR_DEF
        self.minor = self.VER_MINOR_DEF
        self.build = self.VER_BUILD_DEF
        self.date = 0
        return

    def increment_build(self, amount=1):
        # increment the BUILD number
        self.build += amount

    def make_list_date(self) -> list:
        """
        #define FW_DATE_STR = "2020-08-19"
        #define FW_BUILD_STR = "2020-08-19T13:38:33"

        :return: list of lines for file
        """
        self.date = datetime.now(timezone.utc)
        data = list()
        data.append(f"#define {date_str_tag} \"{self.date.year:04d}-{self.date.month:02d}-{self.date.day:02d}\"  ///< BUILD date as text string")
        long_date = self.date.isoformat(timespec='seconds')
        chop = long_date.find("+")
        if chop > 0:
            # "2020-08-19T19:17:38+00:00", but we don't want the '+00:00'
            long_date = long_date[:chop]
        data.append(f"#define {build_str_tag} \"{long_date}\"  ///< BUILD date+time as text string")
        return data

    def make_list_version(self) -> list:
        """
        #define FW_REV_MAJOR = 1
        #define FW_REV_MINOR = 2
        #define FW_REV_BUILD = 99
        #define FW_REV_U32 = 0x01020063
        #define FW_REV_STR = "001.002.00099"

        :return: list of lines for file
        """
        data = list()
        data.append(f"#define {version_major_tag} {self.major}  ///< MAJOR number - defines feature compatibility")
        data.append(f"#define {version_minor_tag} {self.minor}  ///< MINOR number - defines feature tweaks")
        data.append(f"#define {version_build_tag} {self.build}  ///< BUILD number - which spin of MAJOR.MINOR version")
        data.append(f"#define {version_u32_tag} 0x{self.major:02X}{self.minor:02X}{self.build:04X}  ///< Full version as 32-bit unsigned")
        data.append(f"#define {version_str_tag} \"{self.major:03d}.{self.minor:03d}.{self.build:05d}\"  ///< Full version as text string")
        return data

    def make_all(self) -> list:
        data = list()
        data.append("/**")
        data.append("  * @copyright Copyright (c) 2026 Inish Corporation")
        data.append("  *")
        data.append("  * SPDX-License-Identifier: MIT")
        data.append("  *")
        data.append("  * Permission is hereby granted, free of charge, to any person obtaining a")
        data.append("  * copy of this software and associated documentation files (the \"Software\"),")
        data.append("  * to deal in the Software without restriction, including without limitation")
        data.append("  * the rights to use, copy, modify, merge, publish, distribute, sublicense,")
        data.append("  * and/or sell copies of the Software, and to permit persons to whom the")
        data.append("  * Software is furnished to do so, subject to the following conditions:")
        data.append("  *")
        data.append("  * The above copyright notice and this permission notice shall be included in")
        data.append("  * all copies or substantial portions of the Software.")
        data.append("  *")
        data.append("  * THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR")
        data.append("  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,")
        data.append("  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE")
        data.append("  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER")
        data.append("  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING")
        data.append("  * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER")
        data.append("  * DEALINGS IN THE SOFTWARE.")
        data.append("  *")
        data.append("  * @warning - Auto-Generated File - Edit with care!")
        data.append("  *")
        data.append("  * See the 'make_fw_version.py' script")
        data.append("  *")
        data.append("  ******************************************************************************")
        data.append("  */")
        data.append("")
        data.append("#ifndef __APP_VERSION_H")
        data.append("#define __APP_VERSION_H")
        data.append("")
        data.extend(self.make_list_version())
        data.extend(self.make_list_date())
        data.append("")
        data.append("#endif")

        # Append preserved version history
        history = self.import_history()
        if history:
            # Remove closing comment if present
            if history and history[-1].strip() == "*/":
                history = history[:-1]

            # Remove trailing empty lines
            while history and history[-1].strip() == "":
                history = history[:-1]

            data.extend(history)

            # Add new history entry with date and build number
            if self.date:
                history_date = f"{self.date.month:02d}-{self.date.day:02d}-{self.date.year:04d}"
                data.append(f"{history_date}  Build {self.build} - ")
                data.append("")
                data.append("")

            data.append(" */")
        else:
            # Start new history section if none exists
            data.append("")
            data.append("/* VERSION HISTORY (preserved across builds)")
            if self.date:
                history_date = f"{self.date.month:02d}-{self.date.day:02d}-{self.date.year:04d}"
                data.append(f"{history_date}  Build {self.build} - ")
                data.append("")
                data.append("")
            data.append(" */")

        return data

    def print(self):
        data = self.make_all()
        for line in data:
            print(line)
        return

    def export_file(self, filename=None):

        if filename is None:
            filename = version_name

        if os.path.isdir(version_path):
            filename = version_path + '/' + filename

        # self.print()

        data = self.make_all()
        print(f"Exporting APP firmware version file to [{filename}]")
        file_han = open(filename, 'w')
        for line in data:
            file_han.write(line.strip() + '\n')
        file_han.close()
        return

    def import_file(self, filename=None):

        if filename is None:
            filename = version_name

        # print(os.getcwd())
        if os.path.isdir(version_path):
            filename = version_path + '/' + filename

        print(f"Importing APP firmware version file from [{filename}]")
        file_han = open(filename, 'r')
        for line in file_han:
            if line.find(version_major_tag) >= 0:
                # should be like ['#define', 'FW_REV_MAJOR', '=', '1']
                data = line.strip()
                data = data.split()
                self.major = int(data[2])

            elif line.find(version_minor_tag) >= 0:
                # should be like ['#define', 'FW_REV_MINOR', '=', '1']
                data = line.strip()
                data = data.split()
                self.minor = int(data[2])

            elif line.find(version_build_tag) >= 0:
                # should be like ['#define', 'FW_REV_BUILD', '=', '1']
                data = line.strip()
                data = data.split()
                self.build = int(data[2])

        file_han.close()
        return

    def import_history(self, filename=None):
        """Extract version history from after #endif in existing file"""
        if filename is None:
            filename = version_name

        if os.path.isdir(version_path):
            filename = version_path + '/' + filename

        history = []
        capture = False

        try:
            with open(filename, 'r') as f:
                for line in f:
                    # Start capturing after #endif
                    if line.strip() == "#endif":
                        capture = True
                        continue
                    if capture:
                        history.append(line.rstrip())
        except FileNotFoundError:
            pass

        return history


if __name__ == '__main__':

    _obj = FwVersion()

    try:
        _obj.import_file()
    except FileNotFoundError:
        # if not found, we create as FW_REV_STR = "001.001.00001"
        pass

    _obj.increment_build()
    _obj.export_file()

    sys.exit(0)

# Example file created:

# /* APP firmware version file */
# /* Auto-Generated File - edit with care */
#
# #ifndef APP_VERSION_H_
# #define APP_VERSION_H_
#
# #define FW_REV_MAJOR	1
# #define FW_REV_MINOR	1
# #define FW_REV_BUILD	3
# #define FW_REV_U32	0x01010003
# #define FW_REV_STR	"001.001.00003"
#
# #define FW_DATE_STR	"2020-08-19"
# #define FW_ISO_STR	"2020-08-19T21:57:18"
#
# #endif
