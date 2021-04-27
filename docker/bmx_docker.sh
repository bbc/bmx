#!/bin/bash

# This script is a wrapper for running the bmx Docker container. It constructs
# the docker command and passes the remaining arguments to the container.
#
# The user ID and group ID are mapped to that the container writes files using
# those identifiers rather than the root identifier.
#
# The wrapper bind mounts a read-only /input directory and read-write /output
# directory in the container.
#

SCRIPT=$(basename $0)

DEFAULT_DOCKER_IMAGE="bmxtools:latest"
DEFAULT_INPUT="."
DEFAULT_OUTPUT="."

usage()
{
    echo "Usage: ${SCRIPT} <options> [<tool> <args>]"
    echo "<options>:"
    echo "  --help|-h            Print help message and exit"
    echo "  --image <value>      Set the Docker image. Default ${DEFAULT_DOCKER_IMAGE}"
    echo "  --input <value>      Set the input directory to map to /input. Default ${DEFAULT_INPUT}"
    echo "  --output <value>     Set the output directory to map to /output. Default ${DEFAULT_OUTPUT}"
    echo ""
    echo "Example: ${SCRIPT} --output /tmp bmxtranswrap -o /output/out.mxf /input/in.mxf"
}

missing_arg()
{
    usage && echo && echo "Missing argument" && exit 1
}

# If no options are provided then output this tool's usage
[ $# -eq 0 ] && usage && echo ""

DOCKER_IMAGE=${DEFAULT_DOCKER_IMAGE}
INPUT=${DEFAULT_INPUT}
OUTPUT=${DEFAULT_OUTPUT}

while (( "$#" )); do
    case "$1" in
        --help|-h)
            usage
            exit 0
        ;;
        --image)
            DOCKER_IMAGE=$2
            shift 2 || missing_arg
        ;;
        --input)
            INPUT=$2
            shift 2 || missing_arg
        ;;
        --output)
            OUTPUT=$2
            shift 2 || missing_arg
        ;;
        *)
            break
        ;;
    esac
done


MAP_USER="--user $(id -u):$(id -g)"
INPUT_BIND_MOUNT="-v $(realpath ${INPUT}):/input:ro"
OUTPUT_BIND_MOUNT="-v $(realpath ${OUTPUT}):/output"

docker run --rm -ti ${MAP_USER} ${INPUT_BIND_MOUNT} ${OUTPUT_BIND_MOUNT} ${DOCKER_IMAGE} $@
