#!/bin/bash
# to build shaders you need to place a copy of latte-assembler into the current directory
# latte-assembler is part of decaf-emu <https://github.com/decaf-emu/decaf-emu>

cd "${0%/*}"

# colorShader
echo "Building colorShader ..."
./latte-assembler assemble --vsh=colorShader.vsh --psh=colorShader.psh colorShader.gsh
xxd -i colorShader.gsh > colorShader.inc
echo "Done!"

# textureShader
echo "Building textureShader ..."
./latte-assembler assemble --vsh=textureShader.vsh --psh=textureShader.psh textureShader.gsh
xxd -i textureShader.gsh > textureShader.inc
echo "Done!"
