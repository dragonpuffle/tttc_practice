#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

using namespace clang;

namespace {

class UnusedVisitor : public RecursiveASTVisitor<UnusedVisitor> {
public:
  UnusedVisitor(Rewriter &R): TheRewriter(R){}
  
  bool VisitFunctionDecl(FunctionDecl *FD){
    for (auto *param: FD->parameters()){
      insertMU(param);
    }
    return true;
  }

  bool VisitVarDecl(VarDecl *VD){
    if (VD->isLocalVarDecl()){
      insertMU(VD);
    } else if (VD->hasGlobalStorage()){
      insertMU(VD);
    }
    return true;
  }


private:
  Rewriter &TheRewriter;

  void insertMU(VarDecl *VD){
    if (!VD->isUsed() && !VD->isImplicit()){
      SourceLocation SLoc = VD->getBeginLoc();
      if (SLoc.isValid()){
        TheRewriter.InsertText(SLoc, "[[maybe_unused]] ", true, true);
      }
    }
  }
};

class UnusedConsumer : public ASTConsumer {
public:
  UnusedConsumer(Rewriter &R): Visitor(R){}

  void HandleTranslationUnit(ASTContext &Context) override{
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
  

private:
  UnusedVisitor Visitor;
};

class UnusedAction : public PluginASTAction {
protected:
  bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string> &) override {
    return true;
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) override{
    SourceMgr = &CI.getSourceManager();
    TheRewriter.setSourceMgr(*SourceMgr, CI.getLangOpts());
    return std::make_unique<UnusedConsumer>(TheRewriter);
  }

  void EndSourceFileAction() override{
    const auto &srcMgr = TheRewriter.getSourceMgr();
    const OptionalFileEntryRef fileRef = srcMgr.getFileEntryRefForID(srcMgr.getMainFileID());

    if (!fileRef) {
      return;
    }

    std::error_code EC;
    llvm::raw_fd_ostream outFile(fileRef->getName(), EC, llvm::sys::fs::OF_Text);
    if (EC){
      llvm::errs() << "Error while opening file for rewrite" << EC.message() << "\n";
      return;
    }

    TheRewriter.getEditBuffer(SourceMgr->getMainFileID()).write(outFile);
  }

private:
  Rewriter TheRewriter;
  SourceManager *SourceMgr = nullptr;
};

}

static FrontendPluginRegistry::Add<UnusedAction>
    X("mark-unused", "Adds marker [[maybe unused]] for unused variables");
