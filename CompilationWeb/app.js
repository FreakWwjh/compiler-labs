let network = null;
let currentDFA = new DFA();

function showMessage(el, msg, type) {
  el.textContent = msg;
  el.className = 'message ' + type;
  el.style.display = 'block';
}

function hideMessage(el) {
  el.style.display = 'none';
}

function renderGraph() {
  const container = document.getElementById('mynetwork');
  container.innerHTML = '';
  const data = { nodes: [], edges: [] };

  const stateColor = (s) => {
    const isStart = s === currentDFA.startState;
    const isAccept = currentDFA.acceptStates.includes(s);
    if (isStart && isAccept) return { background: '#10b981', border: '#059669' };
    if (isStart) return { background: '#10b981', border: '#059669' };
    if (isAccept) return { background: '#8b5cf6', border: '#7c3aed' };
    return { background: '#f3f4f6', border: '#d1d5db' };
  };

  const fontColor = (s) => {
    const isStart = s === currentDFA.startState;
    const isAccept = currentDFA.acceptStates.includes(s);
    if (isStart || isAccept) return '#ffffff';
    return '#374151';
  };

  for (const s of currentDFA.states) {
    const c = stateColor(s);
    data.nodes.push({
      id: s,
      label: s,
      color: { background: c.background, border: c.border },
      font: { color: fontColor(s), size: 16, face: 'system-ui, -apple-system, sans-serif' },
      borderWidth: 2,
      shape: 'circle',
      size: 30,
      shadow: { enabled: true, color: 'rgba(0,0,0,0.08)', size: 10, x: 2, y: 2 }
    });
  }

  const edgeMap = new Map();
  for (const [state, row] of currentDFA.transitions) {
    for (const [sym, next] of row) {
      const key = `${state}|${next}`;
      if (!edgeMap.has(key)) edgeMap.set(key, []);
      edgeMap.get(key).push(sym);
    }
  }

  for (const [key, labels] of edgeMap) {
    const [from, to] = key.split('|');
    const label = labels.join(', ');
    const isSelfLoop = from === to;
    data.edges.push({
      from,
      to,
      label,
      arrows: { to: { enabled: true, scaleFactor: 0.7 } },
      color: { color: '#6b7280', highlight: '#4f46e5' },
      font: { size: 13, color: '#374151', background: '#ffffff', strokeWidth: 0 },
      smooth: isSelfLoop
        ? { type: 'continuous', roundness: 0.6 }
        : { type: 'continuous', roundness: 0.3 }
    });
  }

  const options = {
    physics: {
      enabled: true,
      solver: 'forceAtlas2Based',
      forceAtlas2Based: {
        gravitationalConstant: -100,
        centralGravity: 0.005,
        springLength: 200,
        springConstant: 0.2,
        damping: 0.4
      },
      stabilization: { iterations: 300 }
    },
    layout: { randomSeed: 42 },
    interaction: { hover: true, zoomView: true, dragView: true }
  };

  network = new vis.Network(container, data, options);
}

document.getElementById('btnParse').addEventListener('click', () => {
  const alphabet = document.getElementById('alphabet').value;
  const states = document.getElementById('states').value;
  const start = document.getElementById('start').value;
  const accept = document.getElementById('accept').value;
  const trans = document.getElementById('transitions').value;

  const ok = currentDFA.parseFromForm(alphabet, states, start, accept, trans);
  const msgEl = document.getElementById('parseMsg');
  if (ok) {
    showMessage(msgEl, '✅ DFA 解析成功且合法！', 'success');
    renderGraph();
  } else {
    showMessage(msgEl, '❌ 解析失败:\n' + currentDFA.errors.join('\n'), 'error');
  }
});

document.getElementById('btnGen').addEventListener('click', () => {
  const n = parseInt(document.getElementById('genN').value);
  const el = document.getElementById('genResult');
  if (isNaN(n) || n < 0) {
    el.innerHTML = '<span class="error-text">N 必须为非负整数。</span>';
    return;
  }
  const results = currentDFA.generateStrings(n);
  if (results.length === 0) {
    el.innerHTML = '<span class="muted">无接受字符串。</span>';
  } else {
    el.innerHTML = `<div class="gen-header">共 ${results.length} 个结果：</div>` +
      results.map(s => `<div class="gen-item">${s}</div>`).join('');
  }
});

document.getElementById('btnTest').addEventListener('click', () => {
  const str = document.getElementById('testString').value;
  const el = document.getElementById('testResult');
  const res = currentDFA.simulate(str);
  if (res.accepted) {
    el.innerHTML = `<span class="success-text">✅ 字符串 "${str}" 被接受 (ACCEPTED)</span>`;
  } else {
    el.innerHTML = `<span class="error-text">❌ 字符串 "${str}" 被拒绝 (REJECTED)<br>原因：${res.reason}</span>`;
  }
});

document.getElementById('fileInput').addEventListener('change', (e) => {
  const file = e.target.files[0];
  if (!file) return;
  const reader = new FileReader();
  reader.onload = (evt) => {
    const text = evt.target.result;
    let ok = false;

    if (text.trim().startsWith('{')) {
      try {
        const json = JSON.parse(text);
        if (json.alphabet) document.getElementById('alphabet').value = json.alphabet.join(' ');
        if (json.states) document.getElementById('states').value = json.states.join(' ');
        if (json.start_state !== undefined) document.getElementById('start').value = json.start_state;
        if (json.accept_states) document.getElementById('accept').value = json.accept_states.join(' ');
        if (json.transitions) {
          const lines = [];
          for (const [state, row] of Object.entries(json.transitions)) {
            for (const [sym, next] of Object.entries(row)) {
              lines.push(`${state} ${sym} ${next}`);
            }
          }
          document.getElementById('transitions').value = lines.join('\n');
        }
        ok = true;
      } catch (_) {}
    }

    if (!ok) {
      ok = currentDFA.parseFromDfaFile(text);
      if (ok) {
        document.getElementById('alphabet').value = currentDFA.alphabet.join(' ');
        document.getElementById('states').value = currentDFA.states.join(' ');
        document.getElementById('start').value = currentDFA.startState;
        document.getElementById('accept').value = currentDFA.acceptStates.join(' ');
        const lines = [];
        for (const [state, row] of currentDFA.transitions) {
          for (const [sym, next] of row) {
            lines.push(`${state} ${sym} ${next}`);
          }
        }
        document.getElementById('transitions').value = lines.join('\n');
      }
    }

    const msgEl = document.getElementById('parseMsg');
    if (ok) {
      showMessage(msgEl, `✅ 文件 "${file.name}" 加载成功，请检查表单后点击解析。`, 'success');
    } else {
      showMessage(msgEl, `⚠️ 无法自动识别文件格式，请手动填写表单。\n${currentDFA.errors.join('\n')}`, 'warning');
    }
  };
  reader.readAsText(file);
});

document.querySelectorAll('.nav-item').forEach(item => {
  item.addEventListener('click', () => {
    document.querySelectorAll('.nav-item').forEach(n => n.classList.remove('active'));
    item.classList.add('active');
    const exp = item.dataset.exp;
    document.querySelectorAll('.panel').forEach(p => p.classList.remove('active'));
    document.getElementById('panel-' + exp).classList.add('active');
  });
});
