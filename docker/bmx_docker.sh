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

DEFAULT_DOCKER_IMAGE="ghcr.io/bbc/bmxtools:latest"
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
    echo "If input and output directories are equal then they also bind mounted to the"
    echo "container's working directory. If the input or output directories are the current"
    echo "directory then they are also bind mounted to the container's working directory."
    echo ""
    echo "Examples:"
    echo "  $ ${SCRIPT} bmxtranswrap -o out.mxf in.mxf"
    echo "  $ ${SCRIPT} --output /tmp bmxtranswrap -o /output/out.mxf in.mxf"
    echo "  $ ${SCRIPT} --input ./sources --output ./dests bmxtranswrap -o /output/out.mxf /input/in.mxf"
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


CONTAINER_WORKDIR="/inout"

MAP_USER_OPT="--user $(id -u):$(id -g)"
WORKDIR_OPT="-w ${CONTAINER_WORKDIR}"
BIND_MOUNT_OPT="-v $(realpath ${INPUT}):/input:ro -v $(realpath ${OUTPUT}):/output"
if [ "$(realpath ${INPUT})" = "$(realpath ${OUTPUT})" ]
then
    BIND_MOUNT_OPT+=" -v $(realpath ${INPUT}):/${CONTAINER_WORKDIR}"
elif [ "$(realpath ${INPUT})" = "$(realpath ./)" ]
then
    BIND_MOUNT_OPT+=" -v $(realpath ${INPUT}):/${CONTAINER_WORKDIR}:ro"
elif [ "$(realpath ${OUTPUT})" = "$(realpath ./)" ]
then
    BIND_MOUNT_OPT+=" -v $(realpath ${OUTPUT}):/${CONTAINER_WORKDIR}"
fi

docker run --rm -ti ${MAP_USER_OPT} ${BIND_MOUNT_OPT} ${WORKDIR_OPT} ${DOCKER_IMAGE} $@
