# Requires shadercross CLI installed from SDL_shadercross
# DXIL compilation is skipped on non-Windows platforms (crashes on Linux)
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*|Windows_NT) COMPILE_DXIL=1 ;;
    *) COMPILE_DXIL=0 ;;
esac

for filename in *.vert.hlsl; do
    if [ -f "$filename" ]; then
        shadercross "$filename" -o "../Compiled/SPIRV/${filename/.hlsl/.spv}"
        shadercross "$filename" -o "../Compiled/MSL/${filename/.hlsl/.msl}"
        [ "$COMPILE_DXIL" -eq 1 ] && shadercross "$filename" -o "../Compiled/DXIL/${filename/.hlsl/.dxil}"
    fi
done

for filename in *.frag.hlsl; do
    if [ -f "$filename" ]; then
        shadercross "$filename" -o "../Compiled/SPIRV/${filename/.hlsl/.spv}"
        shadercross "$filename" -o "../Compiled/MSL/${filename/.hlsl/.msl}"
        [ "$COMPILE_DXIL" -eq 1 ] && shadercross "$filename" -o "../Compiled/DXIL/${filename/.hlsl/.dxil}"
    fi
done

for filename in *.comp.hlsl; do
    if [ -f "$filename" ]; then
        shadercross "$filename" -o "../Compiled/SPIRV/${filename/.hlsl/.spv}"
        shadercross "$filename" -o "../Compiled/MSL/${filename/.hlsl/.msl}"
        [ "$COMPILE_DXIL" -eq 1 ] && shadercross "$filename" -o "../Compiled/DXIL/${filename/.hlsl/.dxil}" || true
    fi
done
#read -p "Press any key to continue" x
exit 0