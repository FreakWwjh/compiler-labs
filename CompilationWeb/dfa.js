class DFA {
  constructor() {
    this.alphabet = [];
    this.states = [];
    this.startState = '';
    this.acceptStates = [];
    this.transitions = new Map(); // state -> Map(symbol -> nextState)
    this.errors = [];
  }

  clear() {
    this.alphabet = [];
    this.states = [];
    this.startState = '';
    this.acceptStates = [];
    this.transitions.clear();
    this.errors = [];
  }

  parseFromForm(alphabetLine, statesLine, startLine, acceptLine, transText) {
    this.clear();
    this.alphabet = alphabetLine.split(/\s+/).filter(s => s.length > 0);
    this.states = statesLine.split(/\s+/).filter(s => s.length > 0);
    this.startState = startLine.trim();
    this.acceptStates = acceptLine.split(/\s+/).filter(s => s.length > 0);

    const lines = transText.split('\n').map(l => l.trim()).filter(l => l.length > 0);
    for (const line of lines) {
      const parts = line.split(/\s+/);
      if (parts.length < 3) {
        this.errors.push(`转移定义格式错误: "${line}"，需要 "状态 字符 下一状态"`);
        continue;
      }
      const [state, sym, next] = parts;
      if (!this.transitions.has(state)) {
        this.transitions.set(state, new Map());
      }
      this.transitions.get(state).set(sym, next);
    }
    return this.validate();
  }

  parseFromDfaFile(text) {
    this.clear();
    const lines = text.split('\n').map(l => l.trim()).filter(l => l.length > 0);
    if (lines.length < 5) {
      this.errors.push('DFA 文件行数不足，至少需要 5 行。');
      return false;
    }

    const hasSpace = lines[0].includes(' ');
    if (hasSpace) {
      this.alphabet = lines[0].split(/\s+/).filter(s => s);
    } else {
      this.alphabet = lines[0].split('').filter(c => !/\s/.test(c));
    }

    const stateCount = parseInt(lines[1]);
    if (isNaN(stateCount) || stateCount <= 0) {
      this.errors.push('状态数量必须为正整数。');
      return false;
    }
    this.states = [];
    for (let i = 1; i <= stateCount; i++) {
      this.states.push(String(i));
    }

    this.startState = lines[2].trim();
    this.acceptStates = lines[3].split(/\s+/).filter(s => s);

    for (let i = 0; i < stateCount; i++) {
      const stateId = String(i + 1);
      const rowLine = lines[4 + i];
      if (!rowLine) {
        this.errors.push(`状态 ${stateId} 的转移行缺失。`);
        return false;
      }
      const rowTokens = rowLine.split(/\s+/).filter(s => s);
      if (rowTokens.length !== this.alphabet.length) {
        this.errors.push(`状态 ${stateId} 的转移表列数不正确，期望 ${this.alphabet.length}，实际 ${rowTokens.length}。`);
        return false;
      }
      const rowMap = new Map();
      for (let j = 0; j < this.alphabet.length; j++) {
        rowMap.set(this.alphabet[j], rowTokens[j]);
      }
      this.transitions.set(stateId, rowMap);
    }
    return this.validate();
  }

  validate() {
    this.errors = [];
    let valid = true;

    if (this.states.length === 0) {
      this.errors.push('状态集为空。');
      valid = false;
    }
    if (this.alphabet.length === 0) {
      this.errors.push('字符集为空。');
      valid = false;
    }
    if (!this.startState) {
      this.errors.push('开始状态未指定。');
      valid = false;
    } else if (!this.states.includes(this.startState)) {
      this.errors.push(`开始状态 "${this.startState}" 不在状态集中。`);
      valid = false;
    }

    if (this.acceptStates.length === 0) {
      this.errors.push('接受状态集为空。');
      valid = false;
    } else {
      for (const s of this.acceptStates) {
        if (!this.states.includes(s)) {
          this.errors.push(`接受状态 "${s}" 不在状态集中。`);
          valid = false;
        }
      }
    }

    for (const s of this.states) {
      if (!this.transitions.has(s)) {
        this.errors.push(`状态 "${s}" 缺少转移定义。`);
        valid = false;
        continue;
      }
      const row = this.transitions.get(s);
      for (const sym of this.alphabet) {
        if (!row.has(sym)) {
          this.errors.push(`状态 "${s}" 缺少字符 "${sym}" 的转移。`);
          valid = false;
        } else {
          const next = row.get(sym);
          if (!this.states.includes(next)) {
            this.errors.push(`状态 "${s}" 在字符 "${sym}" 上的转移目标 "${next}" 不在状态集中。`);
            valid = false;
          }
        }
      }
    }

    for (const [state, row] of this.transitions) {
      if (!this.states.includes(state)) {
        this.errors.push(`转移定义中出现了未定义的状态 "${state}"。`);
        valid = false;
      }
    }

    return valid;
  }

  generateStrings(maxLen) {
    if (maxLen < 0) return [];
    const results = [];
    if (this.acceptStates.includes(this.startState)) {
      results.push('<空串>');
    }

    const queue = [[this.startState, '']];
    let head = 0;
    while (head < queue.length) {
      const [state, str] = queue[head++];
      if (str.length >= maxLen) continue;
      const row = this.transitions.get(state);
      if (!row) continue;
      for (const sym of this.alphabet) {
        const next = row.get(sym);
        if (next === undefined) continue;
        const nextStr = str + sym;
        if (this.acceptStates.includes(next)) {
          results.push(nextStr);
        }
        queue.push([next, nextStr]);
      }
    }
    return results;
  }

  simulate(input) {
    const steps = [];
    let currentState = this.startState;
    steps.push({ state: currentState, sym: null, msg: `初始状态: ${currentState}` });

    for (let i = 0; i < input.length; i++) {
      const ch = input[i];
      if (!this.alphabet.includes(ch)) {
        steps.push({ state: currentState, sym: ch, msg: `字符 '${ch}' 不在字符集中，DFA 崩溃（拒绝）。` });
        return { accepted: false, steps, reason: `非法字符 '${ch}'` };
      }
      const row = this.transitions.get(currentState);
      if (!row || !row.has(ch)) {
        steps.push({ state: currentState, sym: ch, msg: `状态 ${currentState} 对字符 '${ch}' 无转移定义，DFA 崩溃（拒绝）。` });
        return { accepted: false, steps, reason: `无转移: δ(${currentState}, ${ch})` };
      }
      const next = row.get(ch);
      steps.push({ state: next, sym: ch, msg: `δ(${currentState}, '${ch}') = ${next}` });
      currentState = next;
    }

    const accepted = this.acceptStates.includes(currentState);
    return { accepted, steps, endState: currentState };
  }

  toJSONObject() {
    const transObj = {};
    for (const s of this.states) {
      transObj[s] = {};
      const row = this.transitions.get(s);
      if (row) {
        for (const [sym, next] of row) {
          transObj[s][sym] = next;
        }
      }
    }
    return {
      alphabet: this.alphabet,
      states: this.states,
      start_state: this.startState,
      accept_states: this.acceptStates,
      transitions: transObj
    };
  }

  toJSON() {
    return JSON.stringify(this.toJSONObject(), null, 4);
  }
}
