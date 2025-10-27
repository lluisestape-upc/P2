#! /usr/bin/bash

for a1 in $(seq 8 0.1 10); do # Para todos los posibles valores de a1 entre 8 y 12 (con pasos de 0.1)
    for a2 in $(seq 0 0.1 2); do
        echo -ne "$a1 $a2\t" # Imprime el valor de $a1 actual
        scripts/run_vad.sh $a1 $a2 | grep TOTAL # Ejecuta el Test para el valor $a1 actual y imprime su Fscore total (solo imprimiendo la línea con la palabra "TOTAL")
    done
done | sort -t: -k 2n # Ordena resultados por Fscores de mayor a menor