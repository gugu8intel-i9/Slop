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

typedef struct { char* name; SIRId local; SIRType type; char* struct_name; } SlopSIRBinding;
typedef struct { SlopSIRBinding* data; uint32_t len; uint32_t cap; } SlopSIRBindings;

typedef struct { char* name; char* fields[32]; SIRType field_types[32]; uint32_t field_count; } SlopSIRStructDef;
typedef struct { SlopSIRStructDef data[64]; uint32_t len; } SlopSIRStructs;

typedef enum { SLOP_CTX_IF, SLOP_CTX_WHILE } SlopSIRCtxKind;
typedef struct { SlopSIRCtxKind kind; SIRId cond_block; SIRId then_or_body; SIRId else_block; SIRId end_block; bool in_else; } SlopSIRCtx;

typedef struct { SlopSIRCtx data[128]; uint32_t len; } SlopSIRCtxStack;

static inline char* slop_sir_trim(char* s) { while (*s && isspace((unsigned char)*s)) s++; size_t n=strlen(s); while(n&&isspace((unsigned char)s[n-1])) s[--n]=0; return s; }
static inline bool slop_sir_starts(const char* s,const char* p){return strncmp(s,p,strlen(p))==0;}
static inline char* slop_sir_strdup_len(const char* s,size_t n){char* o=(char*)malloc(n+1); if(!o) return NULL; memcpy(o,s,n); o[n]=0; return o;}

static inline void slop_sir_bind_ex(SlopSIRBindings* b,const char* name,SIRId local,SIRType type,const char* struct_name){
    for(uint32_t i=0;i<b->len;i++) if(strcmp(b->data[i].name,name)==0){b->data[i].local=local;b->data[i].type=type;free(b->data[i].struct_name);b->data[i].struct_name=struct_name?strdup(struct_name):NULL;return;}
    if(b->len==b->cap){b->cap=b->cap?b->cap*2:16;b->data=(SlopSIRBinding*)realloc(b->data,(size_t)b->cap*sizeof(SlopSIRBinding));if(!b->data)exit(1);}
    b->data[b->len].name=strdup(name); b->data[b->len].local=local; b->data[b->len].type=type; b->data[b->len].struct_name=struct_name?strdup(struct_name):NULL; b->len++;
}
static inline void slop_sir_bind(SlopSIRBindings* b,const char* name,SIRId local,SIRType type){ slop_sir_bind_ex(b,name,local,type,NULL); }

static inline SlopSIRBinding* slop_sir_find(SlopSIRBindings* b,const char* name){for(uint32_t i=0;i<b->len;i++) if(strcmp(b->data[i].name,name)==0) return &b->data[i]; return NULL;}
static inline void slop_sir_bindings_free(SlopSIRBindings* b){for(uint32_t i=0;i<b->len;i++){ free(b->data[i].name); free(b->data[i].struct_name); } free(b->data); memset(b,0,sizeof(*b));}

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

static inline SlopSIRStructDef* slop_sir_find_struct(SlopSIRStructs* structs, const char* name) {
    for (uint32_t i=0;i<structs->len;i++) if (strcmp(structs->data[i].name,name)==0) return &structs->data[i];
    return NULL;
}

static inline SIRType slop_sir_type_from_name(char* t, SlopSIRStructs* structs) {
    t = slop_sir_trim(t);
    size_t tn = strlen(t);
    while (tn && (t[tn-1] == ',' || isspace((unsigned char)t[tn-1]))) t[--tn] = 0;
    if (strcmp(t, "string") == 0) return SIR_TYPE_STRING;
    if (strcmp(t, "bool") == 0) return SIR_TYPE_BOOL;
    if (strcmp(t, "float") == 0) return SIR_TYPE_F64;
    if (slop_sir_find_struct(structs, t)) return SIR_TYPE_STRUCT;
    return SIR_TYPE_I64;
}

static inline SIRType slop_sir_struct_field_type(SlopSIRStructs* structs, const char* struct_name, const char* field) {
    SlopSIRStructDef* sd = slop_sir_find_struct(structs, struct_name);
    if (!sd) return SIR_TYPE_I64;
    for (uint32_t i=0;i<sd->field_count;i++) if (strcmp(sd->fields[i], field)==0) return sd->field_types[i];
    return SIR_TYPE_I64;
}

static inline void slop_sir_structs_free(SlopSIRStructs* structs) {
    for (uint32_t i=0;i<structs->len;i++) {
        free(structs->data[i].name);
        for (uint32_t j=0;j<structs->data[i].field_count;j++) free(structs->data[i].fields[j]);
    }
    structs->len=0;
}

static inline SIRId slop_sir_lower_expr(SlopLowering* l,SlopSIRBindings* bindings,SlopSIRStructs* structs,char* expr,SIRType* out_type,char** out_struct_name);

static inline uint32_t slop_sir_lower_call_args(SlopLowering* l, SlopSIRBindings* bindings, SlopSIRStructs* structs, char* args, SIRId* out_args, uint32_t max_args) {
    uint32_t count = 0;
    int depth = 0;
    bool in_str = false, esc = false;
    char* start = args;
    for (char* p = args; ; p++) {
        char c = *p;
        if (esc) { esc = false; continue; }
        if (c == '\\' && in_str) { esc = true; continue; }
        if (c == '"') { in_str = !in_str; continue; }
        if (!in_str) {
            if (c == '(' || c == '[') depth++;
            if (c == ')' || c == ']') depth--;
            if ((c == ',' && depth == 0) || c == 0) {
                char saved = *p;
                *p = 0;
                char* part = slop_sir_trim(start);
                if (*part && count < max_args) {
                    SIRType t;
                    out_args[count++] = slop_sir_lower_expr(l, bindings, structs, part, &t, NULL);
                }
                *p = saved;
                if (c == 0) break;
                start = p + 1;
            }
        }
        if (c == 0) break;
    }
    return count;
}


static inline bool slop_sir_is_wrapped(char* expr, char open, char close) {
    expr = slop_sir_trim(expr);
    size_t n = strlen(expr);
    return n >= 2 && expr[0] == open && expr[n - 1] == close;
}

static inline SIRId slop_sir_lower_array_literal(SlopLowering* l, SlopSIRBindings* bindings, SlopSIRStructs* structs, char* expr, SIRType* out_type) {
    expr = slop_sir_trim(expr);
    size_t n = strlen(expr);
    if (n < 2 || expr[0] != '[' || expr[n - 1] != ']') return 0;
    expr[n - 1] = 0;
    char* body = expr + 1;
    SIRId arr = slop_lower_array_new(l, SIR_TYPE_I64);
    char* save = NULL;
    for (char* part = strtok_r(body, ",", &save); part; part = strtok_r(NULL, ",", &save)) {
        SIRType et = SIR_TYPE_VOID;
        SIRId v = slop_sir_lower_expr(l, bindings, structs, part, &et, NULL);
        if (v) slop_lower_array_push(l, arr, v);
    }
    *out_type = SIR_TYPE_ARRAY;
    return arr;
}

static inline SIRId slop_sir_lower_expr(SlopLowering* l,SlopSIRBindings* bindings,SlopSIRStructs* structs,char* expr,SIRType* out_type,char** out_struct_name){
    expr=slop_sir_trim(expr);
    if (slop_sir_starts(expr, "length(")) { char* p = strchr(expr, '('); char* e = strrchr(expr, ')'); if (p && e && e > p) { *e = 0; SIRType at; SIRId av = slop_sir_lower_expr(l, bindings, structs, p + 1, &at, NULL); *out_type = SIR_TYPE_I64; return slop_lower_array_len(l, av); } }
    SIRId arrlit = slop_sir_lower_array_literal(l, bindings, structs, expr, out_type); if (arrlit) return arrlit;
    char* lb = strchr(expr, '['); char* rb = strrchr(expr, ']'); if (lb && rb && rb > lb) { *lb = 0; *rb = 0; char* nm = slop_sir_trim(expr); SlopSIRBinding* bind = slop_sir_find(bindings, nm); if (bind) { SIRType it; SIRId idx = slop_sir_lower_expr(l, bindings, structs, lb + 1, &it, NULL); *out_type = SIR_TYPE_I64; return slop_lower_array_get(l, bind->local, idx, SIR_TYPE_I64); } }
    const char* cmp_ops[]={"==","!=","<=",">=","<",">"};
    char* op=slop_sir_find_op(expr,cmp_ops,6);
    if(op){char opbuf[3]={0}; opbuf[0]=op[0]; if(op[1]=='='||op[0]=='!'||op[0]=='=') opbuf[1]=op[1]; size_t oplen=strlen(opbuf); *op=0; if(oplen==2) op[1]=0; SIRType lt,rt; SIRId a=slop_sir_lower_expr(l,bindings,structs,expr,&lt,NULL); SIRId b=slop_sir_lower_expr(l,bindings,structs,op+oplen,&rt,NULL); *out_type=SIR_TYPE_BOOL; if(strcmp(opbuf,"==")==0)return slop_lower_cmp_eq(l,a,b); if(strcmp(opbuf,"!=")==0)return slop_lower_cmp_ne(l,a,b); if(strcmp(opbuf,"<=")==0)return slop_lower_cmp_le_i64(l,a,b); if(strcmp(opbuf,">=")==0)return slop_lower_cmp_ge_i64(l,a,b); if(strcmp(opbuf,"<")==0)return slop_lower_cmp_lt_i64(l,a,b); return slop_lower_cmp_gt_i64(l,a,b);}
    const char* add_ops[]={"+","-"}; op=slop_sir_find_op(expr,add_ops,2);
    if(op && op!=expr){char opc=*op; *op=0; SIRType lt,rt; SIRId a=slop_sir_lower_expr(l,bindings,structs,expr,&lt,NULL); SIRId b=slop_sir_lower_expr(l,bindings,structs,op+1,&rt,NULL); if(opc=='+'&&(lt==SIR_TYPE_STRING||rt==SIR_TYPE_STRING)){*out_type=SIR_TYPE_STRING;return slop_lower_string_concat(l,a,b);} *out_type=SIR_TYPE_I64; return opc=='+'?slop_lower_add_i64(l,a,b):slop_lower_sub_i64(l,a,b);}
    const char* mul_ops[]={"*","/","%"}; op=slop_sir_find_op(expr,mul_ops,3);
    if(op){char opc=*op; *op=0; SIRType lt,rt; SIRId a=slop_sir_lower_expr(l,bindings,structs,expr,&lt,NULL); SIRId b=slop_sir_lower_expr(l,bindings,structs,op+1,&rt,NULL); *out_type=SIR_TYPE_I64; if(opc=='*')return slop_lower_mul_i64(l,a,b); if(opc=='/')return slop_lower_div_i64(l,a,b); return slop_lower_mod_i64(l,a,b);}

    char* mdot = strchr(expr, '.'); char* mcall = mdot ? strchr(mdot, '(') : NULL; char* mend = mdot ? strrchr(mdot, ')') : NULL; if (mdot && mcall && mend && mend[1] == 0) { *mdot = 0; *mcall = 0; *mend = 0; char* objn = slop_sir_trim(expr); char* mn = slop_sir_trim(mdot + 1); SlopSIRBinding* ob = slop_sir_find(bindings, objn); if (ob && ob->struct_name) { SIRId obj = slop_lower_local_get(l, ob->local, ob->type); SIRId extra[1]={0}; uint32_t ec=slop_sir_lower_call_args(l, bindings, structs, mcall+1, extra, 1); char fname[256]; snprintf(fname, sizeof(fname), "%s_%s", ob->struct_name, mn); SIRId dst=sir_new_value(l->module); SIRInst* inst=sir_emit(l->module,SIR_OP_CALL); inst->type=SIR_TYPE_I64; inst->dst=dst; inst->a=slop_lower_named_id(l,fname); inst->b=obj; inst->c=ec?extra[0]:0; inst->imm=1+ec; *out_type=SIR_TYPE_I64; return dst; } }
    char* dot = strchr(expr, '.'); if (dot) { *dot = 0; char* objn = slop_sir_trim(expr); char* fld = slop_sir_trim(dot + 1); SlopSIRBinding* ob = slop_sir_find(bindings, objn); if (ob) { SIRId obj = slop_lower_local_get(l, ob->local, ob->type); SIRType ft = ob->struct_name ? slop_sir_struct_field_type(structs, ob->struct_name, fld) : SIR_TYPE_I64; *out_type = ft; return slop_lower_field_get(l, obj, fld, ft); } }
    char* callp = strchr(expr, '('); char* calle = strrchr(expr, ')'); if (callp && calle && calle > callp && calle[1] == 0) { *callp = 0; char* cname = slop_sir_trim(expr); SlopSIRStructDef* sd = slop_sir_find_struct(structs, cname); if (sd) { *calle = 0; SIRId obj = slop_lower_struct_new(l, cname, 0); SIRId cargs[16]={0}; uint32_t argc=slop_sir_lower_call_args(l, bindings, structs, callp+1, cargs, 16); for(uint32_t fi=0; fi<argc && fi<sd->field_count; fi++){ slop_lower_field_set(l, obj, sd->fields[fi], cargs[fi]); } *out_type = SIR_TYPE_STRUCT; if(out_struct_name)*out_struct_name=cname; return obj; } *calle = 0; SIRId cargs[2]={0}; uint32_t argc=slop_sir_lower_call_args(l, bindings, structs, callp+1, cargs, 2); SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_CALL); inst->type = SIR_TYPE_I64; inst->dst = dst; inst->a = slop_lower_named_id(l, cname); inst->b = argc > 0 ? cargs[0] : 0; inst->c = argc > 1 ? cargs[1] : 0; inst->imm = argc; *out_type = SIR_TYPE_I64; return dst; }
    if(strcmp(expr,"true")==0){*out_type=SIR_TYPE_BOOL;return slop_lower_bool(l,true);} if(strcmp(expr,"false")==0){*out_type=SIR_TYPE_BOOL;return slop_lower_bool(l,false);}
    char* str=slop_sir_parse_string(expr); if(str){SIRId id=slop_lower_string_literal(l,str); free(str); *out_type=SIR_TYPE_STRING; return id;}
    int64_t iv=0; if(slop_sir_parse_i64(expr,&iv)){*out_type=SIR_TYPE_I64;return slop_lower_i64(l,iv);}
    SlopSIRBinding* b=slop_sir_find(bindings,expr); if(b){*out_type=b->type; if(out_struct_name && b->struct_name)*out_struct_name=b->struct_name; return slop_lower_local_get(l,b->local,b->type);}
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
    sir_module_init(out); SlopLowering lowering; slop_lowering_init(&lowering,out); SlopSIRBindings bindings={0}; SlopSIRStructs structs={0}; SlopSIRCtxStack stack={0}; bool in_struct=false; SlopSIRStructDef* current_struct=NULL;
    bool in_function=false; bool current_main=false;
    char* copy=strdup(source); if(!copy)return false; char* save=NULL; uint32_t line_no=0;
    for(char* line=strtok_r(copy,"\n",&save);line;line=strtok_r(NULL,"\n",&save)){
        line_no++; slop_sir_strip_comment(line); char* t=slop_sir_trim(line); if(!*t||strcmp(t,"{")==0)continue;
        if(in_struct){ if(strcmp(t,"}")==0){in_struct=false;current_struct=NULL;continue;} char* colon=strchr(t, ':'); if(colon && current_struct && current_struct->field_count<32){*colon=0; uint32_t fi=current_struct->field_count; current_struct->fields[fi]=strdup(slop_sir_trim(t)); current_struct->field_types[fi]=slop_sir_type_from_name(colon+1,&structs); current_struct->field_count++;} continue; }
        if(slop_sir_starts(t,"struct ")){char* p=t+7;while(*p&&isspace((unsigned char)*p))p++;char* ns=p;while(*p&&(isalnum((unsigned char)*p)||*p=='_'))p++;if(structs.len<64){current_struct=&structs.data[structs.len++];memset(current_struct,0,sizeof(*current_struct));current_struct->name=slop_sir_strdup_len(ns,(size_t)(p-ns));in_struct=true;}continue;}
        if(slop_sir_starts(t,"fn ")){char* p=t+3;while(*p&&isspace((unsigned char)*p))p++;char* ns=p;while(*p&&(isalnum((unsigned char)*p)||*p=='_'))p++;char* name=slop_sir_strdup_len(ns,(size_t)(p-ns));slop_sir_bindings_free(&bindings);memset(&bindings,0,sizeof(bindings));slop_lower_function_begin(&lowering,name);in_function=true;current_main=(strcmp(name,"main")==0);char* lp=strchr(p,'(');char* rp=strchr(p,')');if(lp&&rp&&rp>lp&&!current_main){*rp=0;char* savep=NULL;for(char* prm=strtok_r(lp+1,",",&savep);prm;prm=strtok_r(NULL,",",&savep)){char* colon=strchr(prm,':');SIRType pt=SIR_TYPE_I64;char* stn=NULL;if(colon){*colon=0;pt=slop_sir_type_from_name(colon+1,&structs); if(pt==SIR_TYPE_STRUCT) stn=slop_sir_trim(colon+1);}char* pn=slop_sir_trim(prm);if(*pn){SIRId name_id=slop_lower_named_id(&lowering,pn);SIRId local=sir_emit_local_declare(lowering.module,name_id,pt,0);slop_sir_bind_ex(&bindings,pn,local,pt,stn);}}}slop_lower_block(&lowering,"entry");free(name);continue;}
        if(slop_sir_line_is_close_else(t)){
            if(!stack.len||stack.data[stack.len-1].kind!=SLOP_CTX_IF){slop_print_diagnostic(diag,(SlopDiagnostic){SLOP_DIAG_ERROR,filename,line_no,1,"else without matching if","put else immediately after an if block",t});goto fail;}
            SlopSIRCtx* c=&stack.data[stack.len-1]; slop_lower_jump(&lowering,c->end_block); slop_sir_emit_block_id(&lowering,c->else_block,"if_else"); c->in_else=true; continue;
        }
        if(strcmp(t,"}")==0){ if(stack.len){ if(!slop_sir_close_context(&lowering,&stack))goto fail; } else if(in_function){ if(current_main) sir_emit_exit(out,0); slop_lower_function_end(&lowering); in_function=false; current_main=false; } continue; }
        if(slop_sir_starts(t,"while ")){char* cond=slop_sir_strip_trailing_open(t+6); SIRId cond_block=slop_sir_new_block_id(&lowering); SIRId body=slop_sir_new_block_id(&lowering); SIRId end=slop_sir_new_block_id(&lowering); slop_lower_jump(&lowering,cond_block); slop_sir_emit_block_id(&lowering,cond_block,"while_cond"); SIRType ct; SIRId cv=slop_sir_lower_expr(&lowering,&bindings,&structs,cond,&ct,NULL); slop_lower_branch(&lowering,cv,body,end); slop_sir_emit_block_id(&lowering,body,"while_body"); stack.data[stack.len++]=(SlopSIRCtx){SLOP_CTX_WHILE,cond_block,body,0,end,false}; continue;}
        if(slop_sir_starts(t,"if ")){char* cond=slop_sir_strip_trailing_open(t+3); SIRId thenb=slop_sir_new_block_id(&lowering); SIRId elseb=slop_sir_new_block_id(&lowering); SIRId end=slop_sir_new_block_id(&lowering); SIRType ct; SIRId cv=slop_sir_lower_expr(&lowering,&bindings,&structs,cond,&ct,NULL); slop_lower_branch(&lowering,cv,thenb,elseb); stack.data[stack.len++]=(SlopSIRCtx){SLOP_CTX_IF,0,thenb,elseb,end,false}; slop_sir_emit_block_id(&lowering,thenb,"if_then"); continue;}
        if(slop_sir_starts(t,"return")){char* rp=t+6;SIRType rt;SIRId rv=slop_sir_lower_expr(&lowering,&bindings,&structs,rp,&rt,NULL);slop_lower_return(&lowering,rv);continue;}
        if(slop_sir_starts(t,"push")){char* p=strchr(t,'(');char* end=strrchr(t,')');if(!p||!end||end<=p){slop_print_diagnostic(diag,(SlopDiagnostic){SLOP_DIAG_ERROR,filename,line_no,1,"expected push(array, value)","example: push(nums, 4)",t});goto fail;}*end=0;p++;char* comma=strchr(p,',');if(!comma){slop_print_diagnostic(diag,(SlopDiagnostic){SLOP_DIAG_ERROR,filename,line_no,1,"expected comma in push", "example: push(nums, 4)", t});goto fail;}*comma=0;char* an=slop_sir_trim(p);SlopSIRBinding* ab=slop_sir_find(&bindings,an);if(!ab){slop_print_diagnostic(diag,slop_diag_unknown_name(filename,line_no,1,an,t));goto fail;}SIRType vt;SIRId val=slop_sir_lower_expr(&lowering,&bindings,&structs,comma+1,&vt,NULL);slop_lower_array_push(&lowering,ab->local,val);continue;}
        if(slop_sir_starts(t,"let ")){char* p=t+4;while(*p&&isspace((unsigned char)*p))p++;char* ns=p;while(*p&&(isalnum((unsigned char)*p)||*p=='_'))p++;char* name=slop_sir_strdup_len(ns,(size_t)(p-ns));while(*p&&*p!='=')p++;if(*p!='='){slop_print_diagnostic(diag,(SlopDiagnostic){SLOP_DIAG_ERROR,filename,line_no,1,"expected '=' in let declaration","write: let name = value",t});free(name);goto fail;}p++;SIRType type; char* stname=NULL; SIRId value=slop_sir_lower_expr(&lowering,&bindings,&structs,p,&type,&stname);if(!value){slop_print_diagnostic(diag,slop_diag_unknown_name(filename,line_no,1,p,t));free(name);goto fail;}SIRId local=slop_lower_let(&lowering,name,type,value);slop_sir_bind_ex(&bindings,name,local,type,stname);free(name);continue;}
        char* eq=strchr(t,'='); if(eq && !(eq>t&&(eq[-1]=='='||eq[1]=='='||eq[-1]=='!'||eq[-1]=='<'||eq[-1]=='>'))){*eq=0; char* name=slop_sir_trim(t); char* dot=strchr(name,'.'); SIRType type; SIRId value=slop_sir_lower_expr(&lowering,&bindings,&structs,eq+1,&type,NULL); if(dot){*dot=0; SlopSIRBinding* ob=slop_sir_find(&bindings,slop_sir_trim(name)); if(!ob){slop_print_diagnostic(diag,slop_diag_unknown_name(filename,line_no,1,name,t));goto fail;} SIRId obj=slop_lower_local_get(&lowering,ob->local,ob->type); slop_lower_field_set(&lowering,obj,slop_sir_trim(dot+1),value); slop_lower_assign(&lowering,ob->local,obj); continue;} SlopSIRBinding* b=slop_sir_find(&bindings,name); if(!b){slop_print_diagnostic(diag,slop_diag_unknown_name(filename,line_no,1,name,t));goto fail;} slop_lower_assign(&lowering,b->local,value); continue;}
        if(slop_sir_starts(t,"print")){char* p=strchr(t,'(');char* end=strrchr(t,')');if(!p||!end||end<=p){slop_print_diagnostic(diag,(SlopDiagnostic){SLOP_DIAG_ERROR,filename,line_no,1,"expected print(expr)","example: print(42)",t});goto fail;}*end=0;p++;SIRType type; char* stname=NULL; SIRId value=slop_sir_lower_expr(&lowering,&bindings,&structs,p,&type,&stname);if(!value){slop_print_diagnostic(diag,slop_diag_unknown_name(filename,line_no,1,p,t));goto fail;} if(type==SIR_TYPE_STRING){SIRId nl=slop_lower_string_literal(&lowering,"\n");SIRId with_nl=slop_lower_string_concat(&lowering,value,nl);slop_lower_print_string_value(&lowering,with_nl);}else slop_lower_print_i64(&lowering,value);continue;}
        slop_print_diagnostic(diag,(SlopDiagnostic){SLOP_DIAG_ERROR,filename,line_no,1,"SIR-first frontend does not support this statement yet","use the stable slop-compiler C backend for full language support",t});goto fail;
    }
    while(stack.len) if(!slop_sir_close_context(&lowering,&stack)) goto fail;
    if(!in_function && out->inst_len == 0) sir_emit_exit(out,0); free(copy); slop_sir_bindings_free(&bindings); slop_sir_structs_free(&structs); return true;
fail: free(copy); slop_sir_bindings_free(&bindings); slop_sir_structs_free(&structs); sir_module_free(out); return false;
}

#endif // SLOP_SIR_FRONTEND_H
