# CSW2CDT computer tape tool suite

Select your desired language / Elija idioma:

- Click [this link for English](#english)

- Pulse [este enlace para Castellano](#castellano)

---

## English

CSW2CDT is a cross-platform encoder-decoder of CSW (v1) 1-bit sample files and CDT/TZX (v1+) tape files. Its purpose is the preservation of single-bit computer tapes in general and 8-bit home computer tapes in particular, with emphasis on the Amstrad CPC and the Sinclair Spectrum, and their shared CDT/TZX tape file format, but also able to encode and decode TSX tapes from the MSX platform; the PZX tape format, a middle point between CSW and CDT/TZX, is secondarily supported as well. Its companion tool CSW0 is an encoder-decoder of RIFF WAVE audio files and CSW (v1) files; both encoders can thus work together to encode extant tapes into tape files, and to decode these files back into audio than can be recorded on real tapes or fed to computers and emulators alike.

The general idea of turning a source tape into a fully encoded CDT/TZX file is to be done in three steps: first, the tape must be sampled into an audio file; second, CSW0 will read that audio file and encode its contents into a partially processed CSW file; third, CSW2CDT will take the CSW file from the second step and parse its data into a fully processed CDT/TZX file that can be examined with editors and fed to emulators. Because noise is a problem in tapes (and even more so as they age and grow old) each stage may need the user to perform multiple retries with different parameters, until the resulting files are good enough to move on to the next stage.

Similarly, the reverse procedure of decoding an extant CDT/TZX file starts by feeding said file to CSW2CDT, who will decode it into a CSW file, and continues by feeding the resulting CSW file to CSW0, who will encode it into a WAV file that can be recorded on a tape or played back on a real machine or an emulator.

Both operations may require user input: removing noise following certain parameters, stating a particular type of tape encoding, etc.

The package also includes CSW2CDT-UI, a Windows binary that simplifies parts of the work by hiding the command line operations: the user can select files and options on a dialog instead of having to type commands and paths in full. The procedures stay the same, however.

---

## Castellano

CSW2CDT es un codificador-decodificador multiplataforma de archivos CSW (v1) muestrados con 1-bit y archivos de cinta CDT/TZX (v1+). Su propósito es la preservación de cintas de ordenador de un solo bit en general y cintas de ordenador doméstico de 8 bits en particular, con énfasis en el Amstrad CPC y el Sinclair Spectrum, y el formato compartiod de archivo de cinta CDT/TZX, aunque también es capaz de codificar y decodificar cintas TSX de la plataforma MSX; el formato de cinta PZX, algo intermedio entre CSW y CDT/TZX, también es soportado de forma secundaria. La herramienta compañera CSW0 es un codificador-decodificador de archivos de audio RIFF WAVE y archivos CSW (v1); ambos codificadores pueden trabajar juntos para codificar cintas existentes en archivos de cinta, y para decodificar estos archivos en audio que puede ser grabado en cintas reales o alimentado a ordenadores y emuladores por igual.

La idea general de convertir una cinta fuente en un archivo CDT/TZX completamente codificado se realiza en tres pasos: primero, la cinta debe ser muestreada en un archivo de audio; segundo, CSW0 leerá ese archivo de audio y codificará su contenido en un archivo CSW parcialmente procesado; tercero, CSW2CDT tomará el archivo CSW del segundo paso y analizará sus datos en un archivo CDT/TZX completamente procesado que pueda ser examinado con editores y alimentado a emuladores. Dado que el ruido es un problema en las cintas (y más aún a medida que envejecen y envejecen) cada etapa puede necesitar que el usuario realice múltiples reintentos con diferentes parámetros, hasta que los archivos resultantes sean lo suficientemente aceptables como para pasar a la siguiente etapa.

Del mismo modo, el procedimiento inverso de descodificación de un archivo CDT/TZX existente comienza alimentando dicho archivo a CSW2CDT, que lo descodificará en un archivo CSW, y continúa alimentando el archivo CSW resultante a CSW0, que lo convertirá en un archivo WAV que puede grabarse en una cinta o reproducirse en una máquina real o en un emulador.

Ambas operaciones pueden requerir la intervención del usuario: eliminar ruido siguiendo determinados parámetros, indicar un tipo concreto de codificación de cinta, etc.

El paquete también incluye CSW2CDT-UI, un binario de Windows que simplifica alguntas partes del trabajo ocultando las operaciones de la línea de comandos: el usuario puede seleccionar archivos y opciones en un cuadro de diálogo en lugar de tener que escribir los comandos y las rutas al completo. Sin embargo, los procedimientos siguen siendo los mismos.

---

## License

This project is licensed under the [GNU GPLv3 Licence](./LICENSE)

Copyright (C) 2020-2023 Cesar Nicolas-Gonzalez

This program comes with ABSOLUTELY NO WARRANTY; for more details
please see the GNU General Public License. This is free software
and you are welcome to redistribute it under certain conditions.

This notice applies to the source code of CSW2CDT and its binaries.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
