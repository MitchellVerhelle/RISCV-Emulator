
    <button id="startBtn">Start Game</button>

    <script>
    const btn     = document.getElementById('startBtn');
    const canvas  = document.getElementById('canvas');
    canvas.style.display = 'none';

    // Emscripten sets this when it’s done loading the wasm
    Module.onRuntimeInitialized = () => {
      btn.onclick = () => {
        btn.style.display    = 'none';
        canvas.style.display = '';
        Module.ccall('start_game');       // now the symbol exists
      };
    };
    </script>