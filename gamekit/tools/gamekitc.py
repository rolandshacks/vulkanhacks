#
# GameKit Compiler
#

from atexit import register
from genericpath import isfile
import sys
import os
import os.path
import getopt
from pathlib import Path
import subprocess

VERBOSE = False

FILENAME_FILTER = [ "CMakeLists.txt" ]
EXTENSION_FILTER = [ ".cpp", ".inc", ".c", ".h" ]
SHADER_EXTENSIONS = [ ".vert", ".frag", ".shader" ]

MAX_LINE_LENGTH = 120
HEXCHARS = "0123456789abcdef"

GLSLC_ENV = "vulkan1.2"
GLSLC_FORMAT = "num"

INDIVIDUAL_DEPENDS_FILE_ENABLED = False

vulkan_sdk_path = None
glslc_executable = None

descriptors = []

def usage():
    print("Usage: gamekitc SOURCE TARGET [NAME]")
    print("")
    print("SOURCE      : Source directory")
    print("TARGET      : Target directory")
    print("NAME        : Target name")

def check_setup():
    '''Check setup'''

    global vulkan_sdk_path
    global glslc_executable

    vulkan_sdk_path = os.getenv('VULKAN_SDK')
    if not vulkan_sdk_path:
        print("No Vulkan SDK installed, please check installation and environment variable VULKAN_SDK")
        sys.exit(3)

    glslc_executable = os.path.normpath(os.path.join(vulkan_sdk_path, "bin", "glslc"))
    if os.name == 'nt':
        glslc_executable += ".exe"

    if not os.path.exists(glslc_executable):
        print("GLSLC compiler not found, please check your Vulkan SDK installation")
        sys.exit(3)

def add_descriptor(source, rel_source, dest, rel_dest, suffix):
    global descriptors
    descriptors.append( (source, rel_source, dest, rel_dest, suffix) )

def get_descriptors():
    global descriptors
    return descriptors

def format_byte(value):
    return "0x" + HEXCHARS[int(value/16)] + HEXCHARS[int(value%16)]

def is_ignored(filePath):
    if filePath.name in FILENAME_FILTER:
        return True
    if filePath.suffix in EXTENSION_FILTER:
        return True
    return False

def is_shader(filePath):
    return filePath.suffix in SHADER_EXTENSIONS

def create_folder(folder):
    if os.path.isdir(folder):
        return
    os.makedirs(folder)

def get_output_filename(source, output_folder, extension):
    path = Path(source)
    depends_file = path.name + extension
    depends_path = os.path.normpath(os.path.join(output_folder, depends_file))
    return depends_path

def binc(source_file, output_file, depends_file):
    '''Convert binary file to C/C++ byte array'''
    buffer_size = 4096
    out_file = open(output_file, "w")
    with open(source_file, "rb") as in_file:
        buffer = b''
        avail = 0
        ofs = 0
        done = False
        line = ""
        while not done:
            if avail < 1:
                buffer = in_file.read(buffer_size)
                if not buffer:
                    break
                avail = len(buffer)
                ofs = 0

            b = buffer[ofs]
            ofs += 1
            avail -= 1

            line += format_byte(b) + ","
            if len(line) >= MAX_LINE_LENGTH:
                out_file.write(line)
                out_file.write("\n")
                line = ""

    if len(line) >= 0:
        out_file.write(line)

    out_file.close()

    if (None != depends_file and len(depends_file) > 0):
        dep_file = open(depends_file, "w")
        dep_file.write(f"{output_file}: {source_file}\n")
        dep_file.close()

    return

def glslc(source_file, output_file, depends_file):
    '''Call glslc executable'''

    args = [
        glslc_executable,
        f"--target-env={GLSLC_ENV}",
        f"-mfmt={GLSLC_FORMAT}"
    ]

    if (None != depends_file and len(depends_file) > 0):
        args.extend([
            "-MD", "-MF", depends_file
        ])

    args.extend([
        "-o", output_file,
        str(source_file)
    ])

    subprocess.run(args)

def needs_update(input, output):

    if not os.path.exists(output):
        return True

    istat = os.stat(input)
    ostat = os.stat(output)

    if istat.st_mtime > ostat.st_mtime:
        return True

    return False

def compile_shader(source, output, depends):
    '''Compile shader file using glslc'''
    if VERBOSE: print(f"compiling shader {source}")
    glslc(source, output, depends)
    return

def compile_data(source, output, depends):
    '''Compile data file'''
    if VERBOSE: print(f"compiling data {source}")
    binc(source, output, depends)
    return

def process_file(source, base_input_folder, output_folder, base_output_folder):
    '''Process file'''

    if VERBOSE: print(f"gamekitc process_file({source}, {output_folder}, {base_output_folder})")

    path = Path(source)

    create_folder(output_folder)
    output = get_output_filename(path, output_folder, ".inc")
    depends = None # get_output_filename(path, output_folder, ".d")

    if needs_update(path, output):
        print(path.name)
        if is_shader(path):
            compile_shader(path, output, depends)
        else:
            compile_data(path, output, depends)
    else:
        if VERBOSE: print(f"no update: {path} {output}")

    rel_input = os.path.relpath(path, base_input_folder)
    rel_output = os.path.relpath(output, base_output_folder)

    add_descriptor(path, rel_input, output, rel_output, path.suffix)

def write_stamp(stamp_file):
    '''Write stamp file'''

    folder = Path(stamp_file).parent
    if not folder.exists:
        os.makedirs(folder)

    f = open(stamp_file, "w")
    f.write(f"GENERATED FILE\n")
    f.close()

def write_depends(depends_file, target, descriptor_file):
    '''Write dependencies file'''
    descriptors = get_descriptors()

    folder = Path(depends_file).parent
    if not folder.exists:
        os.makedirs(folder)

    f = open(depends_file, "w")

    if len(descriptors) > 0:
        f.write(f"{target}:")
        for descriptor in descriptors:
            dependency_path = str(descriptor[3]).replace(' ', '\\ ')
            f.write(f" {dependency_path}")
        f.write("\n")

        for descriptor in descriptors:
            file_target = str(descriptor[3]).replace(' ', '\\ ')
            file_source = str(descriptor[0]).replace('\\', '/').replace(' ', '\\ ')
            f.write(f"{file_target}: {file_source}\n")

    f.close()


def write_descriptor(descriptor_file):
    '''Write dependencies file'''
    descriptors = get_descriptors()

    folder = Path(descriptor_file).parent
    if not folder.exists:
        os.makedirs(folder)

    f = open(descriptor_file, "w")

    f.write("//\n")
    f.write("// GENERATED\n")
    f.write("//\n\n")
    f.write("#include \"gamekit/gamekit.h\"\n\n")
    f.write("using namespace gamekit;\n\n")
    f.write("#include <cstdint>\n")
    f.write("#include <vector>\n")

    idx = 0
    for descriptor in descriptors:
        f.write(f"\n")

        suffix = descriptor[4]

        name = descriptor[1].replace('\\', '/').lower()
        f.write(f"// {name}\n")

        if suffix == ".frag" or suffix == ".vert":
            f.write(f"static const uint32_t data{idx}[] = {{\n")
        else:
            f.write(f"static const uint8_t data{idx}[] = {{\n")

        include_file = descriptor[3].replace('\\', '/')

        f.write(f"    #include <{include_file}>\n")
        f.write("};\n")

        idx += 1

    count = idx
    idx = 0
    f.write("\nstatic const std::vector<gamekit::ResourceDescriptor> descriptors = {\n")
    for descriptor in descriptors:

        typename = "Unknown"

        suffix = descriptor[4]
        if suffix == ".frag": typename = "FragmentShader"
        elif suffix == ".vert": typename = "VertexShader"
        elif suffix == ".png": typename = "Bitmap"
        elif suffix == ".txt": typename = "Text"

        name = descriptor[1].replace('\\', '/').lower()

        f.write(f"    {{ \"{name}\", static_cast<const void*>(data{idx}), sizeof(data{idx}), ResourceType::{typename} }}")
        if idx < count-1: f.write(",")
        f.write("\n")
        idx += 1
    f.write("};\n")

    f.write("\nconst std::vector<gamekit::ResourceDescriptor>& getResourceDescriptors() {\n")
    f.write("    return descriptors;\n")
    f.write("}\n")

    f.close()

def process_folder(input_folder, base_input_folder, output_folder, base_output_folder):
    '''Process folder'''

    if VERBOSE: print(f"gamekitc process_folder({input_folder}, {output_folder}, {base_output_folder})")

    create_folder(output_folder)

    file_names = os.listdir(input_folder)
    for filename in file_names:
        abs_input = os.path.normpath(os.path.join(input_folder, filename))
        source_path = Path(abs_input)
        if source_path.is_dir():
            abs_output = os.path.normpath(os.path.join(output_folder, filename))
            process_folder(abs_input, base_input_folder, abs_output, base_output_folder)
        else:
            path = Path(abs_input)
            if not is_ignored(path):
                process_file(path, base_input_folder, output_folder, base_output_folder)

def process(input, output, name):
    '''Process known folders'''

    cwd = os.getcwd()
    if VERBOSE: print(f"gamekitc {input} {output} {cwd}")

    if not Path(input).is_absolute():
        input = os.path.join(os.getcwd(), input)
    input = os.path.normpath(input)

    if not Path(output).is_absolute():
        output = os.path.join(os.getcwd(), output)
    output = os.path.normpath(output)

    process_folder(input, input, output, output)

    stamp_file = os.path.normpath(os.path.join(output, name + ".stamp"))
    write_stamp(stamp_file)

    descriptor_file = os.path.normpath(os.path.join(output, name + ".cpp"))
    write_descriptor(descriptor_file)

    depends_file = os.path.normpath(os.path.join(output, name + ".d"))
    write_depends(depends_file, name, descriptor_file)


def main():
    '''Main entry'''
    check_setup()
    try:
        opts, args = getopt.getopt(sys.argv[1:], "h:", ["help"])
    except getopt.GetoptError:
        usage()
        sys.exit(2)

    if len(args) < 2:
        usage()
        sys.exit()

    output = "build"
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()

    source = Path(args[0])
    if not source.exists or not os.path.isdir(source):
        print(f"{source} directory does not exist or is invalid")
        sys.exit(3)

    target = Path(args[1])

    name = target.name
    if len(args) >= 3:
        name = args[2]

    process(source, target, name)

if __name__ == "__main__":
    main()
