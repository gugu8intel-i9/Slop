#!/usr/bin/env python3
# Minimal beginner-friendly LSP skeleton: initialize/shutdown + simple diagnostics.
import json, re, sys

def send(msg):
    data=json.dumps(msg)
    sys.stdout.write(f"Content-Length: {len(data)}\r\n\r\n{data}"); sys.stdout.flush()

def read_msg():
    headers={}
    while True:
        line=sys.stdin.buffer.readline()
        if not line: return None
        if line in (b"\r\n", b"\n"): break
        k,v=line.decode().split(":",1); headers[k.lower()]=v.strip()
    body=sys.stdin.buffer.read(int(headers.get('content-length','0')))
    return json.loads(body.decode())

def diagnostics(text):
    out=[]
    for i,line in enumerate(text.splitlines()):
        if 'var ' in line:
            out.append({"range":{"start":{"line":i,"character":line.index('var ')},"end":{"line":i,"character":line.index('var ')+3}},"severity":2,"message":"Slop uses 'let', not 'var'. Hint: let x = ..."})
    return out

docs={}
while True:
    m=read_msg()
    if m is None: break
    method=m.get('method')
    if method=='initialize':
        send({"jsonrpc":"2.0","id":m.get('id'),"result":{"capabilities":{"textDocumentSync":1}}})
    elif method=='shutdown':
        send({"jsonrpc":"2.0","id":m.get('id'),"result":None})
    elif method=='exit':
        break
    elif method=='textDocument/didOpen':
        td=m['params']['textDocument']; uri=td['uri']; docs[uri]=td['text']
        send({"jsonrpc":"2.0","method":"textDocument/publishDiagnostics","params":{"uri":uri,"diagnostics":diagnostics(td['text'])}})
