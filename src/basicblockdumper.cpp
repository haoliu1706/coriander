// Copyright Hugh Perkins 2016

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "basicblockdumper.h"

#include "readIR.h"

#include "EasyCL/util/easycl_stringhelper.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"

#include <iostream>
#include <stdexcept>
#include <sstream>

using namespace std;
using namespace llvm;

namespace cocl {

// maybe this should be somewhere more generic?
string BasicBlockDumper::dumpConstant(Constant *constant) {
    unsigned int valueTy = constant->getValueID();
    ostringstream oss;
    if(ConstantInt *constantInt = dyn_cast<ConstantInt>(constant)) {
        oss << constantInt->getSExtValue();
        string constantintval = oss.str();
        return constantintval;
    } else if(isa<ConstantStruct>(constant)) {
        throw runtime_error("constantStruct not implemented in basicblockdumper.dumpconstnat");
    } else if(ConstantExpr *expr = dyn_cast<ConstantExpr>(constant)) {
        throw runtime_error("constantExpr not implemented in basicblockdumper.dumpconstnat");
        // Instruction *instr = expr->getAsInstruction();
        // copyAddressSpace(constant, instr);
        // string dcires = dumpChainedInstruction(0, instr);
        // // copyAddressSpace(instr, constant);
        // nameByValue[constant] = dcires;
        // return dcires;
    } else if(ConstantFP *constantFP = dyn_cast<ConstantFP>(constant)) {
        return dumpFloatConstant(constantFP);
    } else if(GlobalValue *global = dyn_cast<GlobalValue>(constant)) {
        throw runtime_error("GlobalValue not implemented in basicblockdumper.dumpconstnat");
         // if(PointerType *pointerType = dyn_cast<PointerType>(global->getType())) {
         //     int addressspace = pointerType->getAddressSpace();
         //     string name = getName(global);
         //     if(addressspace == 3) {  // if it's local memory, it's not really 'global', juts return the name
         //         return name;
         //     }
         // }
         // if(nameByValue.find(constant) != nameByValue.end()) {
         //    return nameByValue[constant];
         // }
         // string name = getName(global);
         // string ourinstrstr = "(&" + name + ")";
         // updateAddressSpace(constant, 4);
         // nameByValue[constant] = ourinstrstr;

         // return ourinstrstr;
    } else if(isa<UndefValue>(constant)) {
        return "";
    } else if(isa<ConstantPointerNull>(constant)) {
        return "0";
    } else {
        cout << "valueTy " << valueTy << endl;
        oss << "unknown";
        constant->dump();
        throw runtime_error("unknown constnat type");
    }
    return oss.str();
}

string BasicBlockDumper::dumpOperand(Value *value) {
    if(exprByValue.find(value) != exprByValue.end()) {
        return exprByValue[value];
    }
    if(Constant *constant = dyn_cast<Constant>(value)) {
        return dumpConstant(constant);
    }
    // if(isa<BasicBlock>(value)) {
    //     storeValueName(value);
    //     return nameByValue[value];
    // }
    // if(PHINode *phi = dyn_cast<PHINode>(value)) {
    //     addPHIDeclaration(phi);
    //     string name = nameByValue[value];
    //     return name;
    // }
    // lets just declare it???
    // storeValueName(value);
    // functionNeededForwardDeclarations.insert(value);
    // return nameByValue[value];
    throw runtime_error("Not implemented");
}

std::string BasicBlockDumper::dumpBinaryOperator(BinaryOperator *instr, std::string opstring) {
    string gencode = "";
    Value *op1 = instr->getOperand(0);
    gencode += dumpOperand(op1) + " ";
    gencode += opstring + " ";
    Value *op2 = instr->getOperand(1);
    gencode += dumpOperand(op2);
    return gencode;
}

std::string BasicBlockDumper::dumpIcmp(llvm::ICmpInst *instr) {
    string gencode = "";
    CmpInst::Predicate predicate = instr->getSignedPredicate();  // note: we should detect signedness...
    string predicate_string = "";
    switch(predicate) {
        case CmpInst::ICMP_SLT:
            predicate_string = "<";
            break;
        case CmpInst::ICMP_SGT:
            predicate_string = ">";
            break;
        case CmpInst::ICMP_SGE:
            predicate_string = ">=";
            break;
        case CmpInst::ICMP_SLE:
            predicate_string = "<=";
            break;
        case CmpInst::ICMP_EQ:
            predicate_string = "==";
            break;
        case CmpInst::ICMP_NE:
            predicate_string = "!=";
            break;
        default:
            cout << "predicate " << predicate << endl;
            throw runtime_error("predicate not supported");
    }
    string op0 = dumpOperand(instr->getOperand(0));
    string op1 = dumpOperand(instr->getOperand(1));
    // handle case like `a & 3 == 0`
    if(op0.find('&') == string::npos) {
        op0 = stripOuterParams(op0);
    }
    if(op1.find('&') == string::npos) {
        op1 = stripOuterParams(op1);
    }
    gencode += op0;
    gencode += " " + predicate_string + " ";
    gencode += op1;
    return gencode;
}

std::string BasicBlockDumper::dumpFcmp(llvm::FCmpInst *instr) {
    string gencode = "";
    CmpInst::Predicate predicate = instr->getPredicate();
    string predicate_string = "";
    switch(predicate) {
        case CmpInst::FCMP_ULT:
        case CmpInst::FCMP_OLT:
            predicate_string = "<";
            break;
        case CmpInst::FCMP_UGT:
        case CmpInst::FCMP_OGT:
            predicate_string = ">";
            break;
        case CmpInst::FCMP_UGE:
        case CmpInst::FCMP_OGE:
            predicate_string = ">=";
            break;
        case CmpInst::FCMP_ULE:
        case CmpInst::FCMP_OLE:
            predicate_string = "<=";
            break;
        case CmpInst::FCMP_UEQ:
        case CmpInst::FCMP_OEQ:
            predicate_string = "==";
            break;
        case CmpInst::FCMP_UNE:
        case CmpInst::FCMP_ONE:
            predicate_string = "!=";
            break;
        default:
            cout << "predicate " << predicate << endl;
            throw runtime_error("predicate not supported");
    }
    string op0 = dumpOperand(instr->getOperand(0));
    string op1 = dumpOperand(instr->getOperand(1));
    op0 = stripOuterParams(op0);
    op1 = stripOuterParams(op1);
    gencode += op0;
    gencode += " " + predicate_string + " ";
    gencode += op1;
    return gencode;
}

std::string BasicBlockDumper::dumpBitCast(BitCastInst *instr) {
    string gencode = "";
    string op0str = dumpOperand(instr->getOperand(0));
    if(PointerType *srcType = dyn_cast<PointerType>(instr->getSrcTy())) {
        if(PointerType *destType = dyn_cast<PointerType>(instr->getDestTy())) {
            Type *castType = PointerType::get(destType->getElementType(), srcType->getAddressSpace());
            gencode += "((" + typeDumper->dumpType(castType) + ")" + op0str + ")";
            // copyAddressSpace(instr->getOperand(0), instr);
        }
    } else {
        // just pass through?
        // cout << "bitcast, not a pointer" << endl;
        gencode += "*(" + typeDumper->dumpType(instr->getDestTy()) + " *)&(" + op0str + ")";
    }
    return gencode;
}

std::string BasicBlockDumper::dumpAddrSpaceCast(llvm::AddrSpaceCastInst *instr) {
    string gencode = "";
    string op0str = dumpOperand(instr->getOperand(0));
    // copyAddressSpace(instr->getOperand(0), instr);
    gencode += "((" + typeDumper->dumpType(instr->getType()) + ")" + op0str + ")";
    return gencode;
}

std::string BasicBlockDumper::dumpFPExt(llvm::CastInst *instr) {
    string gencode = "";
    gencode += dumpOperand(instr->getOperand(0));
    return gencode;
}

std::string BasicBlockDumper::dumpZExt(llvm::CastInst *instr) {
    string gencode = "";
    gencode += dumpOperand(instr->getOperand(0));
    return gencode;
}

std::string BasicBlockDumper::dumpSExt(llvm::CastInst *instr) {
    string gencode = "";
    gencode += dumpOperand(instr->getOperand(0));
    return gencode;
}

std::string BasicBlockDumper::dumpFPToUI(llvm::FPToUIInst *instr) {
    string gencode = "";
    string typestr = typeDumper->dumpType(instr->getType());
    // gencode += "(" + typestr + ")" + dumpOperand(instr->getOperand(0));
    gencode += "(*(" + typestr + " *)" + "&" + dumpOperand(instr->getOperand(0)) + ")";
    return gencode;
}

std::string BasicBlockDumper::dumpFPToSI(llvm::FPToSIInst *instr) {
    string gencode = "";
    string typestr = typeDumper->dumpType(instr->getType());
    gencode += "(*(" + typestr + " *)" + "&" + dumpOperand(instr->getOperand(0)) + ")";
    return gencode;
}

std::string BasicBlockDumper::dumpUIToFP(llvm::UIToFPInst *instr) {
    string gencode = "";
    string typestr = typeDumper->dumpType(instr->getType());
    // gencode += "(" + typestr + ")" + dumpOperand(instr->getOperand(0));
    gencode += "(*(" + typestr + " *)" + "&" + dumpOperand(instr->getOperand(0)) + ")";
    return gencode;
}

std::string BasicBlockDumper::dumpSIToFP(llvm::SIToFPInst *instr) {
    string gencode = "";
    string typestr = typeDumper->dumpType(instr->getType());
    // gencode += "(" + typestr + ")" + dumpOperand(instr->getOperand(0));
    gencode += "(*(" + typestr + " *)" + "&" + dumpOperand(instr->getOperand(0)) + ")";
    return gencode;
}

std::string BasicBlockDumper::dumpFPTrunc(llvm::CastInst *instr) {
    // since this is float point trunc, lets just assume we're going from double to float
    // fix any exceptiosn to this rule later
    string gencode = "";
    string typestr = typeDumper->dumpType(instr->getType());
    gencode += "(" + typestr + ")" + dumpOperand(instr->getOperand(0));
    return gencode;
}

std::string BasicBlockDumper::dumpTrunc(llvm::CastInst *instr) {
    string gencode = "";
    string typestr = typeDumper->dumpType(instr->getType());
    gencode += "(" + typestr + ")" + dumpOperand(instr->getOperand(0));
    return gencode;
}

std::string BasicBlockDumper::dumpReturn(ReturnInst *retInst) {
    std::string gencode = "";
    Value *retValue = retInst->getReturnValue();
    if(retValue != 0) {
        Function *F = retInst->getFunction();
        // copyAddressSpace(retValue, F);
        gencode += "return " + dumpOperand(retValue);
    } else {
        // we still need to have "return" if no value, since some loops terminate with a `return` in the middle
        // of the codeblock.  Or rather, they dont terminate, if we dont write out a `return` :-P
        gencode += "return";
    }
    return gencode;
}

std::string BasicBlockDumper::dumpAlloca(llvm::Instruction *alloca) {
    string gencode = "";
    if(PointerType *allocatypeptr = dyn_cast<PointerType>(alloca->getType())) {
        Type *ptrElementType = allocatypeptr->getPointerElementType();
        std::string typestring = typeDumper->dumpType(ptrElementType);
        int count = readInt32Constant(alloca->getOperand(0));
        if(count == 1) {
            if(ArrayType *arrayType = dyn_cast<ArrayType>(ptrElementType)) {
                int innercount = arrayType->getNumElements();
                Type *elementType = arrayType->getElementType();
                string allocaDeclaration = typeDumper->dumpType(elementType) + " " + 
                    dumpOperand(alloca) + "[" + easycl::toString(innercount) + "]";
                allocaDeclarations.insert(allocaDeclaration);
                // currentFunctionSharedDeclarations += allocaDeclaration;
                return "";
            } else {
                // if the elementType is a pointer, assume its global?
                if(isa<PointerType>(ptrElementType)) {
                    // cout << "dumpAlloca, for pointer" << endl;
                    // find the store
                    int numUses = alloca->getNumUses();
                    // cout << "numUses " << numUses << endl;
                    for(auto it=alloca->user_begin(); it != alloca->user_end(); it++) {
                        User *user = *it;
                        // Value *useValue = use->
                        // cout << "user " << endl;
                        if(StoreInst *store = dyn_cast<StoreInst>(user)) {
                            // cout << " got a store" << endl;
                            // user->dump();
                            // cout << endl;
                            // int storeop0space = cast<PointerType>(store->getOperand(0)->getType())->getAddressSpace();
                            // cout << "addessspace " << storeop0space << endl;
                            // if(storeop0space == 1) {
                                // gencode += "global ";
                                // updateAddressSpace(alloca, 1);
                            // }
                            // copyAddressSpace(user, alloca);
                            // typestring = typeDumper->dumpType(ptrElementType);
                        }
                    }
                    // gencode += "global ";
                    // updateAddressSpace(alloca, 1);
                }
                string allocaDeclaration = gencode + typestring + " " + dumpOperand(alloca) + "[1]";
                // just declare this at the head of th efunction
                // allocaDeclaration += "    " + allocaDeclaration + ";\n";
                allocaDeclarations.insert(allocaDeclaration);
                return "";
            }
        } else {
            throw runtime_error("not implemented: alloca for count != 1");
        }
    } else {
        alloca->dump();
        throw runtime_error("dumpalloca not implemented for non pointer type");
    }
}

std::string BasicBlockDumper::dumpLoad(llvm::LoadInst *instr) {
    string rhs = dumpOperand(instr->getOperand(0)) + "[0]";
    // copyAddressSpace(instr->getOperand(0), instr);
    return rhs;
}

std::string BasicBlockDumper::dumpStore(llvm::StoreInst *instr) {
    string gencode = "";
    string rhs = dumpOperand(instr->getOperand(0));
    rhs = stripOuterParams(rhs);
    gencode += dumpOperand(instr->getOperand(1)) + "[0] = " + rhs;
    return gencode;
}

std::vector<std::string> BasicBlockDumper::dumpInsertValue(llvm::InsertValueInst *instr) {
    // string gencode = "";
    string lhs = "";
    cout << "lhs undef value? " << isa<UndefValue>(instr->getOperand(0)) << endl;
    string incomingOperand = dumpOperand(instr->getOperand(0));
    // if rhs is empty, that means its 'undef', so we better declare it, I guess...
    Type *currentType = instr->getType();
    bool declaredVar = false;
    if(incomingOperand == "") {
        variablesToDeclare.insert(instr);
        // string declaration = typeDumper->dumpType(instr->getType()) + " " + dumpOperand(instr) + ";\n";
        // currentFunctionSharedDeclarations += declaration;
        // gencode += "    ";
        incomingOperand = dumpOperand(instr);
        declaredVar = true;
    }
    lhs += incomingOperand;
    ArrayRef<unsigned> indices = instr->getIndices();
    int numIndices = instr->getNumIndices();
    for(int d=0; d < numIndices; d++) {
        int idx = indices[d];
        Type *newType = 0;
        if(currentType->isPointerTy() || isa<ArrayType>(currentType)) {
            if(d == 0) {
                if(isa<ArrayType>(currentType->getPointerElementType())) {
                    lhs = "(&" + lhs + ")";
                }
            }
            lhs += string("[") + dumpOperand(instr->getOperand(d + 1)) + "]";
            newType = currentType->getPointerElementType();
        } else if(StructType *structtype = dyn_cast<StructType>(currentType)) {
            string structName = getName(structtype);
            if(structName == "struct.float4") {
                Type *elementType = structtype->getElementType(idx);
                Type *castType = PointerType::get(elementType, 0);
                newType = elementType;
                lhs = "((" + typeDumper->dumpType(castType) + ")&" + lhs + ")";
                lhs += string("[") + easycl::toString(idx) + "]";
            } else {
                // generic struct
                Type *elementType = structtype->getElementType(idx);
                lhs += string(".f") + easycl::toString(idx);
                newType = elementType;
            }
        } else {
            currentType->dump();
            throw runtime_error("type not implemented in insertvalue");
        }
        currentType = newType;
    }
    std::vector<std::string> res;
    // gencode += lhs + " = " + dumpOperand(instr->getOperand(1));
    res.push_back(lhs + " = " + dumpOperand(instr->getOperand(1)));
    if(!declaredVar) {
        variablesToDeclare.insert(instr);
        // res.push_back(typeDumper->dumpType(instr->getType()) + " " + dumpOperand(instr) + " = " + incomingOperand);
        res.push_back(dumpOperand(instr) + " = " + incomingOperand);
    }
    return res;
    // if(!declaredVar) {
    //     gencode += "    " + dumpType(instr->getType()) + " " + dumpOperand(instr) + " = " + incomingOperand + ";\n";
    // }
    //     gencode += "    " + dumpType(instr->getType()) + " " + dumpOperand(instr) + " = " + incomingOperand + ";\n";
    // return gencode;
}

std::string BasicBlockDumper::dumpExtractValue(llvm::ExtractValueInst *instr) {
    string gencode = "";
    string lhs = "";
    string incomingOperand = dumpOperand(instr->getAggregateOperand());
    // if rhs is empty, that means its 'undef', so we better declare it, I guess...
    Type *currentType = instr->getAggregateOperand()->getType();
    lhs += incomingOperand;
    ArrayRef<unsigned> indices = instr->getIndices();
    int numIndices = instr->getNumIndices();
    for(int d=0; d < numIndices; d++) {
        int idx = indices[d];
        Type *newType = 0;
        if(currentType->isPointerTy() || isa<ArrayType>(currentType)) {
            if(d == 0) {
                if(isa<ArrayType>(currentType->getPointerElementType())) {
                    lhs = "(&" + lhs + ")";
                }
            }
            lhs += string("[") + dumpOperand(instr->getOperand(d + 1)) + "]";
            newType = currentType->getPointerElementType();
        } else if(StructType *structtype = dyn_cast<StructType>(currentType)) {
            string structName = getName(structtype);
            if(structName == "struct.float4") {
                Type *elementType = structtype->getElementType(idx);
                Type *castType = PointerType::get(elementType, 0);
                newType = elementType;
                lhs = "((" + typeDumper->dumpType(castType) + ")&" + lhs + ")";
                lhs += string("[") + easycl::toString(idx) + "]";
            } else {
                // generic struct
                Type *elementType = structtype->getElementType(idx);
                lhs += string(".f") + easycl::toString(idx);
                newType = elementType;
            }
        } else {
            currentType->dump();
            throw runtime_error("type not implemented in extractvalue");
        }
        currentType = newType;
    }
    gencode += lhs;
    return gencode;
}

string BasicBlockDumper::dumpInstruction(string indent, Instruction *instruction) {
    auto opcode = instruction->getOpcode();
    string resultName = localNames->getOrCreateName(instruction);
    // string 
    // storeValueName(instruction);
    // string resultName = localNames->getOrCreateName(instruction);
    // string resultName = exprByValue[instruction];
    exprByValue[instruction] = resultName;
    string resultType = typeDumper->dumpType(instruction->getType());

    string gencode = "";
    string instructionCode = "";
    // if(debug) {
        // COCL_PRINT(cout << resultType << " " << resultName << " =");
        // COCL_PRINT(cout << " " << string(instruction->getOpcodeName()));
        // for(auto it=instruction->op_begin(); it != instruction->op_end(); it++) {
        //     Value *op = &*it->get();
        //     COCL_PRINT(cout << " " << dumpOperand(op));
        // }
        // COCL_PRINT(cout << endl);
    // }
    // lets dump the original isntruction, commented out
    string originalInstruction ="";
    originalInstruction += resultType + " " + resultName + " =";
    originalInstruction += " " + string(instruction->getOpcodeName());
    for(auto it=instruction->op_begin(); it != instruction->op_end(); it++) {
        Value *op = &*it->get();
        originalInstruction += " ";
        string originalName = localNames->getNameOrEmpty(op);
        if(originalName == "") {
            originalName = "<unk";
        }
        originalInstruction += originalName;
        // if(origNameByValue.find(op) != exprByValue.end()) {
        //     originalInstruction += exprByValue[op];
        // } else {
        //     originalInstruction += "<unk>";
        // }
    }
    vector<string> reslines;
    // gencode += "/* " + originalInstruction + " */\n    ";
    switch(opcode) {
        case Instruction::FAdd:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "+");
            break;
        case Instruction::FSub:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "-");
            break;
        case Instruction::FMul:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "*");
            break;
        case Instruction::FDiv:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "/");
            break;
        case Instruction::Sub:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "-");
            break;
        case Instruction::Add:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "+");
            break;
        case Instruction::Mul:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "*");
            break;
        case Instruction::SDiv:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "/");
            break;
        case Instruction::UDiv:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "/");
            break;
        case Instruction::SRem:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "%");
            break;
        case Instruction::And:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "&");
            break;
        case Instruction::Or:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "|");
            break;
        case Instruction::Xor:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "^");
            break;
        case Instruction::LShr:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), ">>");
            break;
        case Instruction::Shl:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), "<<");
            break;
        case Instruction::AShr:
            instructionCode = dumpBinaryOperator(cast<BinaryOperator>(instruction), ">>");
            break;
        case Instruction::ICmp:
            instructionCode = dumpIcmp(cast<ICmpInst>(instruction));
            break;
        case Instruction::FCmp:
            instructionCode = dumpFcmp(cast<FCmpInst>(instruction));
            break;
        case Instruction::SExt:
            instructionCode = dumpSExt(cast<CastInst>(instruction));
            break;
        case Instruction::ZExt:
            instructionCode = dumpZExt(cast<CastInst>(instruction));
            break;
        case Instruction::FPExt:
            instructionCode = dumpFPExt(cast<CastInst>(instruction));
            break;
        case Instruction::FPTrunc:
            instructionCode = dumpFPTrunc(cast<CastInst>(instruction));
            break;
        case Instruction::Trunc:
            instructionCode = dumpTrunc(cast<CastInst>(instruction));
            break;
        case Instruction::UIToFP:
            instructionCode = dumpUIToFP(cast<UIToFPInst>(instruction));
            break;
        case Instruction::SIToFP:
            instructionCode = dumpSIToFP(cast<SIToFPInst>(instruction));
            break;
        case Instruction::FPToUI:
            instructionCode = dumpFPToUI(cast<FPToUIInst>(instruction));
            break;
        case Instruction::FPToSI:
            instructionCode = dumpFPToSI(cast<FPToSIInst>(instruction));
            break;
        case Instruction::BitCast:
            instructionCode = dumpBitCast(cast<BitCastInst>(instruction));
            break;
        case Instruction::AddrSpaceCast:
            instructionCode = dumpAddrSpaceCast(cast<AddrSpaceCastInst>(instruction));
            break;
        // case Instruction::PtrToInt:
        //     instructionCode = dumpPtrToInt(cast<PtrToIntInst>(instruction));
        //     break;
        // case Instruction::IntToPtr:
        //     instructionCode = dumpInttoPtr(cast<IntToPtrInst>(instruction));
        //     break;
        // case Instruction::Select:
        //     // COCL_PRINT(cout << "its a select" << endl);
        //     instructionCode = dumpSelect(cast<SelectInst>(instruction));
        //     break;
        // case Instruction::GetElementPtr:
        //     instructionCode = dumpGetElementPtr(cast<GetElementPtrInst>(instruction));
        //     break;
        case Instruction::InsertValue:
            reslines = dumpInsertValue(cast<InsertValueInst>(instruction));
            // string gencode = "";
            for(auto it=reslines.begin(); it != reslines.end(); it++) {
                instructionCode += indent + *it + ";\n";
            }
            return instructionCode;
            // return indent + instructionCode + ";\n";
            // break;
        case Instruction::ExtractValue:
            instructionCode = dumpExtractValue(cast<ExtractValueInst>(instruction));
            break;
        case Instruction::Store:
            instructionCode = dumpStore(cast<StoreInst>(instruction));
            break;
        // case Instruction::Call:
        //     instructionCode = dumpCall(cast<CallInst>(instruction));
        //     break;
        case Instruction::Load:
            instructionCode = dumpLoad(cast<LoadInst>(instruction));
            break;
        case Instruction::Alloca:
            instructionCode = dumpAlloca(cast<AllocaInst>(instruction));
            return "";
        // case Instruction::Br:
        //     instructionCode = dumpBranch(cast<BranchInst>(instruction));
        //     return instructionCode;
        //     // break;
        case Instruction::Ret:
            instructionCode = dumpReturn(cast<ReturnInst>(instruction));
            return instructionCode + ";\n";
            // break;
        // case Instruction::PHI:
        //     addPHIDeclaration(cast<PHINode>(instruction));
        //     return "";
        //     // break;
        default:
            cout << "opcode string " << instruction->getOpcodeName() << endl;
            throw runtime_error("unknown opcode");
    }
    string typestr = typeDumper->dumpType(instruction->getType());
    Use *use = 0;
    User *use_user = 0;
    bool weArePointer = isa<PointerType>(instruction->getType());
    bool useIsPointer = false;
    bool useIsAStore = false;
    bool useIsExtractValue = false;
    bool useIsAPhi = false;
    if(instruction->hasOneUse()) {
        use = &*instruction->use_begin();
        use_user = use->getUser();
        useIsAStore = isa<StoreInst>(use_user);
        useIsPointer = isa<PointerType>(use_user->getType());
        useIsExtractValue = isa<ExtractValueInst>(use_user);
        useIsAPhi = isa<PHINode>(use_user);
    }
    if(!useIsAPhi && !useIsExtractValue && instruction->hasOneUse()) { // } && !useIsAStore) {
        if(!isSingleExpression(instructionCode)) {
            instructionCode= "(" + instructionCode + ")";
        }
        exprByValue[instruction] = instructionCode;
        // nameByValue[instruction] = instructionCode;
        if(_addIRToCl) {
            return "/* " + originalInstruction + " */\n" + indent;
        } else {
            return "";
        }
        // return "";
    } else {
        if(_addIRToCl) {
            gencode += "/* " + originalInstruction + " */\n" + indent;
        }
        if(instructionCode != "") {
            gencode += indent;
            if(typestr != "void") {
                instructionCode = stripOuterParams(instructionCode);
                // functionNeededForwardDeclarations.insert(instruction);
                // variablesToDeclare[instruction] = resultName;
                variablesToDeclare.insert(instruction);
                gencode += dumpOperand(instruction) + " = ";
            }
            gencode += instructionCode + ";\n";
        }
    }
    return gencode;
}

std::string BasicBlockDumper::getAllocaDeclarations(string indent) {
    string gencode = "";
    for(auto it=allocaDeclarations.begin(); it != allocaDeclarations.end(); it++) {
        string declaration = *it;
        gencode += indent + declaration + ";\n";
    }
    return gencode;
}

std::string BasicBlockDumper::writeDeclarations(std::string indent) {
    string gencode = "";
    for(auto it=variablesToDeclare.begin(); it != variablesToDeclare.end(); it++) {
        Value *value = *it;
        // value->dump();
        string declaration = typeDumper->dumpType(value->getType()) + " " + localNames->getName(value);
        gencode += indent + declaration + ";\n";
    }
    return gencode;
}

std::string BasicBlockDumper::toCl() {
    string gencode = "";
    for(auto it = block->begin(); it != block->end(); it++) {
        Instruction *inst = &*it;
        if(isa<PHINode>(inst) || isa<BranchInst>(inst) || isa<ReturnInst>(inst)) {
            continue;
        }
        inst->dump();
        // cout << endl;
        string instructionCode = dumpInstruction("    ", inst);
        cout << "instructionCode [" << instructionCode << "]" << endl;
        gencode += instructionCode;
    }
    return gencode;
}

} // namespace cocl