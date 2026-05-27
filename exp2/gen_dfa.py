import json

letters_lower = [chr(ord('a') + i) for i in range(26)]
letters_upper = [chr(ord('A') + i) for i in range(26)]
digits = [str(i) for i in range(10)]

states = list(range(0, 31))
start_state = 0

# accept_states: state -> token type
accept_states = {
    "1": "ID",
    "2": "NUM",
    "3": "ASG",
    "4": "EQ",
    "5": "LT",
    "6": "LE",
    "7": "GT",
    "8": "GE",
    "10": "NE",
    "11": "ADD",
    "12": "SUB",
    "13": "MUL",
    "14": "DIV",
    "15": "SEMI",
    "16": "LPAR",
    "17": "RPAR",
    "18": "LBR",
    "19": "RBR",
    "20": "LBK",
    "21": "RBK",
    "22": "CMA",
    "25": "FLOAT",
    "26": "FLOAT",
    "27": "FLOAT",
}

alphabet = set()
transitions = {str(s): {} for s in states}

# Helper to add transitions
def add_trans(state, sym, next_state):
    transitions[str(state)][sym] = next_state
    alphabet.add(sym)

# State 0: initial
for ch in letters_lower + letters_upper:
    add_trans(0, ch, 1)
for d in digits:
    add_trans(0, d, 2)
add_trans(0, '=', 3)
add_trans(0, '<', 5)
add_trans(0, '>', 7)
add_trans(0, '!', 9)
add_trans(0, '+', 11)
add_trans(0, '-', 12)
add_trans(0, '*', 13)
add_trans(0, '/', 14)
add_trans(0, ';', 15)
add_trans(0, '(', 16)
add_trans(0, ')', 17)
add_trans(0, '{', 18)
add_trans(0, '}', 19)
add_trans(0, '[', 20)
add_trans(0, ']', 21)
add_trans(0, ',', 22)
add_trans(0, '.', 23)

# State 1: ID (accept)
for ch in letters_lower + letters_upper + digits:
    add_trans(1, ch, 1)

# State 2: NUM (accept) -> digit loops, and . -> 24
for d in digits:
    add_trans(2, d, 2)
add_trans(2, '.', 24)

# State 3: ASG (accept) -> = -> EQ(4)
add_trans(3, '=', 4)

# State 5: LT (accept) -> = -> LE(6)
add_trans(5, '=', 6)

# State 7: GT (accept) -> = -> GE(8)
add_trans(7, '=', 8)

# State 9: ! -> = -> NE(10)
add_trans(9, '=', 10)

# State 23: . -> digit -> FLOAT_dot(27)
for d in digits:
    add_trans(23, d, 27)

# State 24: after NUM.  -> digit -> FLOAT_frac(25)
for d in digits:
    add_trans(24, d, 25)

# State 25: FLOAT (accept) -> digit loops
for d in digits:
    add_trans(25, d, 25)

# State 27: FLOAT for .digits (accept) -> digit loops
for d in digits:
    add_trans(27, d, 27)

# Scientific notation: (NUM or FLOAT) followed by e/E
# State 28: after e/E (not accept)
# State 29: after +/- in exponent (not accept)
# State 30: FLOAT_exp (accept)
for src_state in [2, 25, 27]:
    add_trans(src_state, 'e', 28)
    add_trans(src_state, 'E', 28)

add_trans(28, '+', 29)
add_trans(28, '-', 29)
for d in digits:
    add_trans(28, d, 30)
    add_trans(29, d, 30)
    add_trans(30, d, 30)

accept_states["30"] = "FLOAT"

# Build final JSON
dfa = {
    "alphabet": sorted(alphabet),
    "states": states,
    "start_state": start_state,
    "accept_states": accept_states,
    "transitions": transitions
}

with open('/home/openEuler/workspace/exp2/dfa_lex.json', 'w', encoding='utf-8') as f:
    json.dump(dfa, f, indent=2, ensure_ascii=False)

print("Generated dfa_lex.json with", len(states), "states and", len(alphabet), "symbols")
