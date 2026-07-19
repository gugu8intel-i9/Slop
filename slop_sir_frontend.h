#ifndef SLOP_SIR_FRONTEND_H
#define SLOP_SIR_FRONTEND_H

// SIR-first Slop frontend slice.
// Real .slop -> SIR path for a growing core language subset. It fails closed
// with diagnostics for unsupported syntax instead of silently faking support.
// Supported now: main wrapper, let, assignment, int/string/bool literals,
// + - * / %, comparisons, print, if/else, while, and string concat.

#include "slop_lowering.h"
#include "slop_diagnostics.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char* name; SIRId local; SIRType type; } SlopSIRBinding;
typedef struct { SlopSIRBinding* data; uint32_t len; uint32_t cap; } SlopSIRBindings;

typedef enum { SLOP_CTX_IF, SLOP_CTX_WHILE } SlopSIRCtxKind;
typedef struct { SlopSIRCtxKind kind; SIRId cond_block; SIRId then_or_body; SIRId else_block; SIRId end_block; bool in_else; } SlopSIRCtx;

typedef struct { SlopSIRCtx data[128]; uint32_t len; } SlopSIRCtxStack;

static inline char* slop_sir_trim(char* s) { while (*s && isspace((unsigned char)*s)) s++; size_t n=strlen(s); while(n&&isspace((unsigned char)s[n-1])) s[--n]=0; return s; }
static inline bool slop_sir_starts(const char* s,const char* p){return strncmp(s,p,strlen(p))==0;}
static inline char* slop_sir_strdup_len(const char* s,size_t n){char* o=(char*)malloc(n+1); if(!o) return NULL; memcpy(o,s,n); o[n]=0; return o;}

static inline void slop_sir_bind(SlopSIRBindings* b,const char* name,SIRId local,SIRType type){
    for(uint32_t i=0;i<b->len;i++) if(strcmp(b->data[i].name,name)==0){b->data[i].local=local;b->data[i].type=type;return;}
    if(b->len==b->cap){b->cap=b->cap?b->cap*2:16;b->data=(SlopSIRBinding*)realloc(b->data,(size_t)b->cap*sizeof(SlopSIRBinding));if(!b->data)exit(1);}
    b->data[b->len].name=strdup(name); b->data[b->len].local=local; b->data[b->len].type=type; b->len++;
}
static inline SlopSIRBinding* slop_sir_find(SlopSIRBindings* b,const char* name){for(uint32_t i=0;i<b->len;i++) if(strcmp(b->data[i].name,name)==0) return &b->data[i]; return NULL;}
static inline void slop_sir_bindings_free(SlopSIRBindings* b){for(uint32_t i=0;i<b->len;i++) free(b->data[i].name); free(b->data); memset(b,0,sizeof(*b));}

static inline void slop_sir_strip_comment(char* s){bool in=false,esc=false;for(size_t i=0;s[i];i++){char c=s[i];if(esc){esc=false;continue;} if(c=='\\'&&in){esc=true;continue;} if(c=='"'){in=!in;continue;} if(c=='#'&&!in){s[i]=0;return;}}}

static inline char* slop_sir_parse_string(const char* e){while(*e&&isspace((unsigned char)*e))e++; if(*e!='"')return NULL; e++; char* out=(char*)malloc(strlen(e)+1); if(!out)return NULL; uint32_t j=0; bool esc=false; for(;*e;e++){char c=*e; if(esc){if(c=='n')out[j++]='\n';else if(c=='t')out[j++]='\t';else if(c=='r')out[j++]='\r';else out[j++]=c; esc=false;} else if(c=='\\')esc=true; else if(c=='"'){out[j]=0;return out;} else out[j++]=c;} free(out); return NULL;}
static inline bool slop_sir_parse_i64(const char* e,int64_t* out){while(*e&&isspace((unsigned char)*e))e++; char* end=NULL; long long v=strtoll(e,&end,10); if(end==e)return false; while(*end&&isspace((unsigned char)*end))end++; if(*end)return false; *out=(int64_t)v; return true;}

static inline char* slop_sir_find_op(char* expr,const char** ops,uint32_t op_count){
    int depth=0; bool in=false,esc=false; char* found=NULL;
    for(char* p=expr;*p;p++){
        if(esc){esc=false;continue;} if(*p=='\\'&&in){esc=true;continue;} if(*p=='"'){in=!in;continue;} if(in)continue;
        if(*p=='(')depth++; else if(*p==')')depth--; if(depth)continue;
        for(uint32_t i=0;i<op_count;i++){size_t n=strlen(ops[i]); if(strncmp(p,ops[i],n)==0) found=p;}
    }
    return found;
}

static inline SIRId slop_sir_lower_expr(SlopLowering* l,SlopSIRBindings* bindings,char* expr,SIRType* out_type){
    expr=slop_sir_trim(expr);
    const char* cmp_ops[]={"==","!=","<=",">=","<",">"};
    char* op=slop_sir_find_op(expr,cmp_ops,6);
    if(op){char opbuf[3]={0}; opbuf[0]=op[0]; if(op[1]=='='||op[0]=='!'||op[0]=='=') opbuf[1]=op[1]; size_t oplen=strlen(opbuf); *op=0; if(oplen==2) op[1]=0; SIRType lt,rt; SIRId a=slop_sir_lower_expr(l,bindings,expr,&lt); SIRId b=slop_sir_lower_expr(l,bindings,op+oplen,&rt); *out_type=SIR_TYPE_BOOL; if(strcmp(opbuf,"==")==0)return slop_lower_cmp_eq(l,a,b); if(strcmp(opbuf,"!=")==0)return slop_lower_cmp_ne(l,a,b); if(strcmp(opbuf,"<=")==0)return slop_lower_cmp_le_i64(l,a,b); if(strcmp(opbuf,">=")==0)return slop_lower_cmp_ge_i64(l,a,b); if(strcmp(opbuf,"<")==0)return slop_lower_cmp_lt_i64(l,a,b); return slop_lower_cmp_gt_i64(l,a,b);}
    const char* add_ops[]={"+","-"}; op=slop_sir_find_op(expr,add_ops,2);
    if(op && op!=expr){char opc=*op; *op=0; SIRType lt,rt; SIRId a=slop_sir_lower_expr(l,bindings,expr,&lt); SIRId b=slop_sir_lower_expr(l,bindings,op+1,&rt); if(opc=='+'&&(lt==SIR_TYPE_STRING||rt==SIR_TYPE_STRING)){*out_type=SIR_TYPE_STRING;return slop_lower_string_concat(l,a,b);} *out_type=SIR_TYPE_I64; return opc=='+'?slop_lower_add_i64(l,a,b):slop_lower_sub_i64(l,a,b);}
    const char* mul_ops[]={"*","/","%"}; op=slop_sir_find_op(expr,mul_ops,3);
    if(op){char opc=*op; *op=0; SIRType lt,rt; SIRId a=slop_sir_lower_expr(l,bindings,expr,&lt); SIRId b=slop_sir_lower_expr(l,bindings,op+1,&rt); *out_type=SIR_TYPE_I64; if(opc=='*')return slop_lower_mul_i64(l,a,b); if(opc=='/')return slop_lower_div_i64(l,a,b); return slop_lower_mod_i64(l,a,b);}
    if(strcmp(expr,"true")==0){*out_type=SIR_TYPE_BOOL;return slop_lower_bool(l,true);} if(strcmp(expr,"false")==0){*out_type=SIR_TYPE_BOOL;return slop_lower_bool(l,false);}
    char* str=slop_sir_parse_string(expr); if(str){SIRId id=slop_lower_string_literal(l,str); free(str); *out_type=SIR_TYPE_STRING; return id;}
    int64_t iv=0; if(slop_sir_parse_i64(expr,&iv)){*out_type=SIR_TYPE_I64;return slop_lower_i64(l,iv);}
    SlopSIRBinding* b=slop_sir_find(bindings,expr); if(b){*out_type=b->type; return slop_lower_local_get(l,b->local,b->type);}
    *out_type=SIR_TYPE_VOID; return 0;
}

static inline char* slop_sir_strip_trailing_open(char* t){char* b=strrchr(t,'{'); if(b)*b=0; return slop_sir_trim(t);}
static inline bool slop_sir_line_is_close_else(char* t){return slop_sir_starts(t,"} else");}


static inline SIRId slop_sir_new_block_id(SlopLowering* l) { return sir_new_value(l->module); }
static inline void slop_sir_emit_block_id(SlopLowering* l, SIRId id, const char* label) {
    SIRInst* inst = sir_emit(l->module, SIR_OP_BLOCK);
    inst->dst = id;
    inst->a = sir_add_string(l->module, label, strlen(label));
    l->current_block = id;
}

static inline bool slop_sir_close_context(SlopLowering* l,SlopSIRCtxStack* stack){
    if(!stack->len) return true; SlopSIRCtx* c=&stack->data[stack->len-1];
    if(c->kind==SLOP_CTX_WHILE){slop_lower_jump(l,c->cond_block); slop_sir_emit_block_id(l,c->end_block,"while_end"); stack->len--; return true;}
    if(c->kind==SLOP_CTX_IF){slop_lower_jump(l,c->end_block); if(!c->in_else) slop_sir_emit_block_id(l,c->else_block,"if_else"); slop_sir_emit_block_id(l,c->end_block,"if_end"); stack->len--; return true;}
    return false;
}

static inline bool slop_sir_lower_source(const char* filename,const char* source,SIRModule* out,FILE* diag){
    sir_module_init(out); SlopLowering lowering; slop_lowering_init(&lowering,out); SlopSIRBindings bindings={0}; SlopSIRCtxStack stack={0};
    slop_lower_function_begin(&lowering,"main"); slop_lower_block(&lowering,"entry");
    char* copy=strdup(source); if(!copy)return false; char* save=NULL; uint32_t line_no=0;
    for(char* line=strtok_r(copy,"\n",&save);line;line=strtok_r(NULL,"\n",&save)){
        line_no++; slop_sir_strip_comment(line); char* t=slop_sir_trim(line); if(!*t||slop_sir_starts(t,"fn ")||strcmp(t,"{")==0)continue;
        if(slop_sir_line_is_close_else(t)){
            if(!stack.len||stack.data[stack.len-1].kind!=SLOP_CTX_IF){slop_print_diagnostic(diag,(SlopDiagnostic){SLOP_DIAG_ERROR,filename,line_no,1,"else without matching if","put else immediately after an if block",t});goto fail;}
            SlopSIRCtx* c=&stack.data[stack.len-1]; slop_lower_jump(&lowering,c->end_block); slop_sir_emit_block_id(&lowering,c->else_block,"if_else"); c->in_else=true; continue;
        }
        if(strcmp(t,"}")==0){ if(!slop_sir_close_context(&lowering,&stack))goto fail; continue; }
        if(slop_sir_starts(t,"while ")){char* cond=slop_sir_strip_trailing_open(t+6); SIRId cond_block=slop_sir_new_block_id(&lowering); SIRId body=slop_sir_new_block_id(&lowering); SIRId end=slop_sir_new_block_id(&lowering); slop_lower_jump(&lowering,cond_block); slop_sir_emit_block_id(&lowering,cond_block,"while_cond"); SIRType ct; SIRId cv=slop_sir_lower_expr(&lowering,&bindings,cond,&ct); slop_lower_branch(&lowering,cv,body,end); slop_sir_emit_block_id(&lowering,body,"while_body"); stack.data[stack.len++]=(SlopSIRCtx){SLOP_CTX_WHILE,cond_block,body,0,end,false}; continue;}
        if(slop_sir_starts(t,"if ")){char* cond=slop_sir_strip_trailing_open(t+3); SIRId thenb=slop_sir_new_block_id(&lowering); SIRId elseb=slop_sir_new_block_id(&lowering); SIRId end=slop_sir_new_block_id(&lowering); SIRType ct; SIRId cv=slop_sir_lower_expr(&lowering,&bindings,cond,&ct); slop_lower_branch(&lowering,cv,thenb,elseb); stack.data[stack.len++]=(SlopSIRCtx){SLOP_CTX_IF,0,thenb,elseb,end,false}; slop_sir_emit_block_id(&lowering,thenb,"if_then"); continue;}
        if(slop_sir_starts(t,"let ")){char* p=t+4;while(*p&&isspace((unsigned char)*p))p++;char* ns=p;while(*p&&(isalnum((unsigned char)*p)||*p=='_'))p++;char* name=slop_sir_strdup_len(ns,(size_t)(p-ns));while(*p&&*p!='=')p++;if(*p!='='){slop_print_diagnostic(diag,(SlopDiagnostic){SLOP_DIAG_ERROR,filename,line_no,1,"expected '=' in let declaration","write: let name = value",t});free(name);goto fail;}p++;SIRType type;SIRId value=slop_sir_lower_expr(&lowering,&bindings,p,&type);if(!value){slop_print_diagnostic(diag,slop_diag_unknown_name(filename,line_no,1,p,t));free(name);goto fail;}SIRId local=slop_lower_let(&lowering,name,type,value);slop_sir_bind(&bindings,name,local,type);free(name);continue;}
        char* eq=strchr(t,'='); if(eq && !(eq>t&&(eq[-1]=='='||eq[1]=='='||eq[-1]=='!'||eq[-1]=='<'||eq[-1]=='>'))){*eq=0; char* name=slop_sir_trim(t); SlopSIRBinding* b=slop_sir_find(&bindings,name); if(!b){slop_print_diagnostic(diag,slop_diag_unknown_name(filename,line_no,1,name,t));goto fail;} SIRType type; SIRId value=slop_sir_lower_expr(&lowering,&bindings,eq+1,&type); slop_lower_assign(&lowering,b->local,value); continue;}
        if(slop_sir_starts(t,"print")){char* p=strchr(t,'(');char* end=strrchr(t,')');if(!p||!end||end<=p){slop_print_diagnostic(diag,(SlopDiagnostic){SLOP_DIAG_ERROR,filename,line_no,1,"expected print(expr)","example: print(42)",t});goto fail;}*end=0;p++;SIRType type;SIRId value=slop_sir_lower_expr(&lowering,&bindings,p,&type);if(!value){slop_print_diagnostic(diag,slop_diag_unknown_name(filename,line_no,1,p,t));goto fail;} if(type==SIR_TYPE_STRING){SIRId nl=slop_lower_string_literal(&lowering,"\n");SIRId with_nl=slop_lower_string_concat(&lowering,value,nl);slop_lower_print_string_value(&lowering,with_nl);}else slop_lower_print_i64(&lowering,value);continue;}
        slop_print_diagnostic(diag,(SlopDiagnostic){SLOP_DIAG_ERROR,filename,line_no,1,"SIR-first frontend does not support this statement yet","use the stable slop-compiler C backend for full language support",t});goto fail;
    }
    while(stack.len) if(!slop_sir_close_context(&lowering,&stack)) goto fail;
    sir_emit_exit(out,0); free(copy); slop_sir_bindings_free(&bindings); return true;
fail: free(copy); slop_sir_bindings_free(&bindings); sir_module_free(out); return false;
}

#endif // SLOP_SIR_FRONTEND_H
