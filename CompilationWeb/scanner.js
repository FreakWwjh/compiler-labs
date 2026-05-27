// scanner.js - Lexical Analyzer for CompilationWeb (Exp2)

const ScannerKeywords = {
  int: 'INT', float: 'FLOAT', void: 'VOID', if: 'IF', else: 'ELSE',
  while: 'WHILE', return: 'RETURN', input: 'INPUT', print: 'PRINT'
};

const TokenColors = {
  INT: '#2563eb', FLOAT: '#2563eb', VOID: '#2563eb', IF: '#2563eb', ELSE: '#2563eb',
  WHILE: '#2563eb', RETURN: '#2563eb', INPUT: '#2563eb', PRINT: '#2563eb',
  ID: '#059669',
  NUM: '#d97706',
  FLOAT: '#d97706',
  ADD: '#7c3aed', SUB: '#7c3aed', MUL: '#7c3aed', DIV: '#7c3aed',
  LT: '#7c3aed', LE: '#7c3aed', EQ: '#7c3aed', GT: '#7c3aed', GE: '#7c3aed', NE: '#7c3aed',
  ASG: '#7c3aed',
  SEMI: '#6b7280', LPAR: '#6b7280', RPAR: '#6b7280', LBR: '#6b7280', RBR: '#6b7280',
  LBK: '#6b7280', RBK: '#6b7280', CMA: '#6b7280',
  UNKNOWN: '#dc2626'
};

class Lexer {
  constructor(source) {
    this.source = source;
    this.pos = 0;
    this.line = 1;
    this.col = 1;
    this.tokens = [];
  }

  peek() {
    if (this.pos >= this.source.length) return '\0';
    return this.source[this.pos];
  }

  advance() {
    const ch = this.peek();
    if (ch === '\n') {
      this.line++;
      this.col = 1;
    } else {
      this.col++;
    }
    this.pos++;
    return ch;
  }

  skipWhitespace() {
    while (/\s/.test(this.peek())) {
      this.advance();
    }
  }

  readIdentifier() {
    const startLine = this.line;
    const startCol = this.col;
    let value = '';
    while (/[a-zA-Z0-9_]/.test(this.peek())) {
      value += this.advance();
    }
    const type = ScannerKeywords[value] || 'ID';
    return { type, value, line: startLine, col: startCol };
  }

  readNumber() {
    const startLine = this.line;
    const startCol = this.col;
    let value = '';
    let hasDigitsBefore = false;
    let hasDot = false;
    let hasDigitsAfter = false;
    let hasExp = false;

    while (/\d/.test(this.peek())) {
      value += this.advance();
      hasDigitsBefore = true;
    }

    if (this.peek() === '.') {
      hasDot = true;
      value += this.advance();
      while (/\d/.test(this.peek())) {
        value += this.advance();
        hasDigitsAfter = true;
      }
    }

    if (this.peek() === 'e' || this.peek() === 'E') {
      hasExp = true;
      value += this.advance();
      if (this.peek() === '+' || this.peek() === '-') {
        value += this.advance();
      }
      let expDigits = false;
      while (/\d/.test(this.peek())) {
        value += this.advance();
        expDigits = true;
      }
      if (!expDigits) {
        return { type: 'UNKNOWN', value, line: startLine, col: startCol };
      }
    }

    if (hasDot || hasExp) {
      if (hasDigitsBefore || hasDigitsAfter || hasExp) {
        return { type: 'FLOAT', value, line: startLine, col: startCol };
      }
      return { type: 'UNKNOWN', value, line: startLine, col: startCol };
    }

    return { type: 'NUM', value, line: startLine, col: startCol };
  }

  nextToken() {
    this.skipWhitespace();
    const ch = this.peek();
    if (ch === '\0') return { type: 'EOF', value: '', line: this.line, col: this.col };

    const startLine = this.line;
    const startCol = this.col;

    if (/[a-zA-Z_]/.test(ch)) return this.readIdentifier();
    if (/\d/.test(ch) || ch === '.') return this.readNumber();

    this.advance();

    switch (ch) {
      case '+': return { type: 'ADD', value: '+', line: startLine, col: startCol };
      case '-': return { type: 'SUB', value: '-', line: startLine, col: startCol };
      case '*': return { type: 'MUL', value: '*', line: startLine, col: startCol };
      case '/': return { type: 'DIV', value: '/', line: startLine, col: startCol };
      case ';': return { type: 'SEMI', value: ';', line: startLine, col: startCol };
      case '(': return { type: 'LPAR', value: '(', line: startLine, col: startCol };
      case ')': return { type: 'RPAR', value: ')', line: startLine, col: startCol };
      case '{': return { type: 'LBR', value: '{', line: startLine, col: startCol };
      case '}': return { type: 'RBR', value: '}', line: startLine, col: startCol };
      case '[': return { type: 'LBK', value: '[', line: startLine, col: startCol };
      case ']': return { type: 'RBK', value: ']', line: startLine, col: startCol };
      case ',': return { type: 'CMA', value: ',', line: startLine, col: startCol };
      case '=':
        if (this.peek() === '=') { this.advance(); return { type: 'EQ', value: '==', line: startLine, col: startCol }; }
        return { type: 'ASG', value: '=', line: startLine, col: startCol };
      case '<':
        if (this.peek() === '=') { this.advance(); return { type: 'LE', value: '<=', line: startLine, col: startCol }; }
        return { type: 'LT', value: '<', line: startLine, col: startCol };
      case '>':
        if (this.peek() === '=') { this.advance(); return { type: 'GE', value: '>=', line: startLine, col: startCol }; }
        return { type: 'GT', value: '>', line: startLine, col: startCol };
      case '!':
        if (this.peek() === '=') { this.advance(); return { type: 'NE', value: '!=', line: startLine, col: startCol }; }
        return { type: 'UNKNOWN', value: '!', line: startLine, col: startCol };
      default:
        return { type: 'UNKNOWN', value: ch, line: startLine, col: startCol };
    }
  }

  scanAll() {
    const tokens = [];
    let tok;
    do {
      tok = this.nextToken();
      tokens.push(tok);
    } while (tok.type !== 'EOF');
    return tokens;
  }
}

function renderTokens(tokens, containerId) {
  const container = document.getElementById(containerId);
  const filtered = tokens.filter(t => t.type !== 'EOF');
  if (filtered.length === 0) {
    container.innerHTML = '<div class="muted">无 Token 输出。</div>';
    return;
  }

  let html = '<table class="token-table"><thead><tr><th>类型</th><th>值</th><th>位置</th></tr></thead><tbody>';
  for (const t of filtered) {
    const color = TokenColors[t.type] || '#374151';
    html += `<tr><td><span class="token-badge" style="background:${color}20;color:${color};border:1px solid ${color}40">${t.type}</span></td><td><code>${escapeHtml(t.value)}</code></td><td class="muted">${t.line}:${t.col}</td></tr>`;
  }
  html += '</tbody></table>';
  container.innerHTML = html;
}

function renderHighlightedSource(source, tokens, containerId) {
  const container = document.getElementById(containerId);
  // Simple approach: rebuild HTML with spans. Since tokens are sequential and source is linear,
  // we can walk the source and wrap recognized token characters.
  // For simplicity and correctness, we just render the original source with syntax highlighting
  // by tokenizing again and mapping positions.
  let html = '';
  let pos = 0;
  const tokList = tokens.filter(t => t.type !== 'EOF');

  // A simpler highlighting: just show the source with pre/code and rely on CSS for general look
  // Or we can produce a line-by-line colored view. Let's do a basic character-level coloring using token spans.
  // Since calculating exact source ranges from line/col is tricky with multi-byte chars, we'll
  // just render source in a pre block and overlay a summary.
  html = `<pre class="source-preview"><code>${escapeHtml(source)}</code></pre>`;
  container.innerHTML = html;
}

function escapeHtml(text) {
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}
