#!/bin/bash

# Lanzamos el test con "scripts/run_vad.sh $1 $2" ($1 y $2 son argumentos numéricos de entrada)
ALPHA1=${1:-8.15} # ${1:-num} si queremos añadir un valor default
ALPHA2=${2:-1.15} # ${2:-num} si queremos añadir un valor default
TO_VOICE=${3:-8}
TO_SILENCE=${4:-17}

# Be sure that this file has execution permissions:
# Use the nautilus explorer or chmod +x run_vad.sh

# Establecemos que el código de retorno de un pipeline sea el del último programa con código de retorno
# distinto de cero, o cero si todos devuelven cero.
set -o pipefail

# Write here the name and path of your program and database
DIR_P2=$HOME/PAV/P2
DB=$DIR_P2/db.v4
CMD="$DIR_P2/bin/vad --alpha1 $ALPHA1 --alpha2 $ALPHA2 --to-voice $TO_VOICE --to-silence $TO_SILENCE" # Comando del script que se ejecutará

for filewav in $DB/*/*wav; do
#    echo
    echo "**************** $filewav ****************"
    if [[ ! -f $filewav ]]; then 
	    echo "Wav file not found: $filewav" >&2
	    exit 1
    fi

    filevad=${filewav/.wav/.vad}

    $CMD -i $filewav -o $filevad || exit 1

# Alternatively, uncomment to create output wave files
#    filewavOut=${filewav/.wav/.vad.wav}
#    $CMD $filewav $filevad $filewavOut || exit 1

done

scripts/vad_evaluation.pl $DB/*/*lab

exit 0
