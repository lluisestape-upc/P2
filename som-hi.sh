#! /usr/bin/bash

# Vamos a ver qué combinación de "alpha1" y "alpha2" es la mejor
# for a1 in $(seq 8 0.05 9); do # Para todos los posibles valores de a1 entre 8 y 9 (con pasos de 0.05)
#     for a2 in $(seq 0.5 0.05 1.5); do # Para todos los posibles valores de a2 entre 0.5 y 1.5 (con pasos de 0.05)
#         echo -ne "$a1 $a2\t" # Imprime el valor de $a1 y $a2 actual
#         scripts/run_vad.sh $a1 $a2 | grep TOTAL # Ejecuta el Test para los valores $a1/$a2 actuales y imprime su Fscore total (solo imprimiendo la línea con la palabra "TOTAL")
#     done
# done | sort -t: -k 2n # Ordena resultados por Fscores de mayor a menor

# Con la mejor combinación a1/a2, vamos a ver los mejores valores para "to-voice" y "to-silence"
# for tv in $(seq 1 1 20); do # Para todos los posibles valores de to_voice entre 1 y 20
#     for ts in $(seq 1 1 20); do # Para todos los posibles valores de to_silence entre 1 y 20
#         echo -ne "$tv $ts\t" # Imprime el valor de $tv y $ts actual
#         scripts/run_vad.sh 8.15 1.15 $tv $ts | grep TOTAL # Ejecuta el Test para los valores $tv/$ts actuales y imprime su Fscore total (solo imprimiendo la línea con la palabra "TOTAL")
#     done
# done | sort -t: -k 2n # Ordena resultados por Fscores de mayor a menor

# Ya tenemos los mejores "to-voice" y "to-silence", vamos a ver si podemos mejorar con "to-init" (volvemos a probar "a1" y "a2" al estar relacionados con el valor de referencia)
for a1 in $(seq 8 0.1 8.5); do # Para todos los posibles valores de a1 entre 8 y 8.5 (con pasos de 0.05)
    for a2 in $(seq 0.8 0.1 1.3); do # Para todos los posibles valores de a2 entre 0.8 y 1.3 (con pasos de 0.05)
        for ti in $(seq 1 1 10); do # Para todos los posibles valores de ti entre 1 y 10
            echo -ne "$a1 $a2 $ti\t" # Imprime el valor de $a1, $a2 y $ti actual
            scripts/run_vad.sh $a1 $a2 8 17 $ti | grep TOTAL # Ejecuta el Test para los valores $a1/$a2/$ti actuales y imprime su Fscore total (solo imprimiendo la línea con la palabra "TOTAL")
        done
    done
done | sort -t: -k 2n # Ordena resultados por Fscores de mayor a menor