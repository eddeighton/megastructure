// Copyright (c) 2005-2011 Code Synthesis Tools CC
//
// This program was generated by CodeSynthesis XSD/e, an XML Schema
// to C++ data binding compiler for embedded systems.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
//
//

// Begin prologue.
//
//
// End prologue.

#include "mega_schema-pimpl.hxx"

#include <xsde/cxx/parser/validating/string-common.hxx>

namespace megaxml
{
  // Package_pimpl
  //

  Package_pimpl::
  Package_pimpl (bool b)
  {
    this->Package_pimpl_base_ = b;
    this->Package_pimpl_state_.Package_ = 0;
  }

  Package_pimpl::
  ~Package_pimpl ()
  {
    if (!this->Package_pimpl_base_ && this->Package_pimpl_state_.Package_)
      delete this->Package_pimpl_state_.Package_;
  }

  void Package_pimpl::
  _reset ()
  {
    Package_pskel::_reset ();

    if (!this->Package_pimpl_base_ && this->Package_pimpl_state_.Package_)
    {
      delete this->Package_pimpl_state_.Package_;
      this->Package_pimpl_state_.Package_ = 0;
    }
  }

  void Package_pimpl::
  pre_impl (::megaxml::Package* x)
  {
    this->Package_pimpl_state_.Package_ = x;
  }

  void Package_pimpl::
  pre ()
  {
    ::megaxml::Package* x = new ::megaxml::Package;
    this->pre_impl (x);
  }

  void Package_pimpl::
  Name (const ::std::string& x)
  {
    this->Package_pimpl_state_.Package_->Name (x);
  }

  void Package_pimpl::
  Repository (const ::std::string& x)
  {
    this->Package_pimpl_state_.Package_->Repository (x);
  }

  void Package_pimpl::
  License (const ::std::string& x)
  {
    this->Package_pimpl_state_.Package_->License (x);
  }

  void Package_pimpl::
  Description (const ::std::string& x)
  {
    this->Package_pimpl_state_.Package_->Description (x);
  }

  void Package_pimpl::
  Directories (::megaxml::Directories* x)
  {
    this->Package_pimpl_state_.Package_->Directories (x);
  }

  void Package_pimpl::
  Files (::megaxml::Files* x)
  {
    this->Package_pimpl_state_.Package_->Files (x);
  }

  void Package_pimpl::
  Command (const ::std::string& x)
  {
    this->Package_pimpl_state_.Package_->Command ().push_back (x);
  }

  ::megaxml::Package* Package_pimpl::
  post_Package ()
  {
    ::megaxml::Package* r = this->Package_pimpl_state_.Package_;
    this->Package_pimpl_state_.Package_ = 0;
    return r;
  }

  // Host_pimpl
  //

  Host_pimpl::
  Host_pimpl (bool b)
  : Host_pskel (&base_impl_),
    base_impl_ (true)
  {
    this->Host_pimpl_base_ = b;
    this->Host_pimpl_state_.Host_ = 0;
  }

  Host_pimpl::
  ~Host_pimpl ()
  {
    if (!this->Host_pimpl_base_ && this->Host_pimpl_state_.Host_)
      delete this->Host_pimpl_state_.Host_;
  }

  void Host_pimpl::
  _reset ()
  {
    Host_pskel::_reset ();

    if (!this->Host_pimpl_base_ && this->Host_pimpl_state_.Host_)
    {
      delete this->Host_pimpl_state_.Host_;
      this->Host_pimpl_state_.Host_ = 0;
    }
  }

  void Host_pimpl::
  pre_impl (::megaxml::Host* x)
  {
    this->Host_pimpl_state_.Host_ = x;
    this->base_impl_.pre_impl (x);
  }

  void Host_pimpl::
  pre ()
  {
    ::megaxml::Host* x = new ::megaxml::Host;
    this->pre_impl (x);
  }

  ::megaxml::Host* Host_pimpl::
  post_Host ()
  {
    this->base_impl_.post_Package ();
    ::megaxml::Host* r = this->Host_pimpl_state_.Host_;
    this->Host_pimpl_state_.Host_ = 0;
    return r;
  }

  // Stack_pimpl
  //

  void Stack_pimpl::
  pre ()
  {
    this->Stack_pimpl_state_.Stack_ = ::megaxml::Stack ();
  }

  void Stack_pimpl::
  Size (unsigned int x)
  {
    this->Stack_pimpl_state_.Stack_.Size (x);
  }

  ::megaxml::Stack Stack_pimpl::
  post_Stack ()
  {
    return this->Stack_pimpl_state_.Stack_;
  }

  // Fibers_pimpl
  //

  void Fibers_pimpl::
  pre ()
  {
    this->Fibers_pimpl_state_.Fibers_ = ::megaxml::Fibers ();
  }

  void Fibers_pimpl::
  Stack (const ::megaxml::Stack& x)
  {
    this->Fibers_pimpl_state_.Fibers_.Stack (x);
  }

  ::megaxml::Fibers Fibers_pimpl::
  post_Fibers ()
  {
    return this->Fibers_pimpl_state_.Fibers_;
  }

  // Defaults_pimpl
  //

  void Defaults_pimpl::
  pre ()
  {
    this->Defaults_pimpl_state_.Defaults_ = ::megaxml::Defaults ();
  }

  void Defaults_pimpl::
  Fibers (const ::megaxml::Fibers& x)
  {
    this->Defaults_pimpl_state_.Defaults_.Fibers (x);
  }

  ::megaxml::Defaults Defaults_pimpl::
  post_Defaults ()
  {
    return this->Defaults_pimpl_state_.Defaults_;
  }

  // Build_pimpl
  //

  void Build_pimpl::
  pre ()
  {
    this->Build_pimpl_state_.Build_ = ::megaxml::Build ();
  }

  void Build_pimpl::
  Name (const ::std::string& x)
  {
    this->Build_pimpl_state_.Build_.Name (x);
  }

  void Build_pimpl::
  CompilerFlags (const ::std::string& x)
  {
    this->Build_pimpl_state_.Build_.CompilerFlags (x);
  }

  void Build_pimpl::
  LinkerFlags (const ::std::string& x)
  {
    this->Build_pimpl_state_.Build_.LinkerFlags (x);
  }

  ::megaxml::Build Build_pimpl::
  post_Build ()
  {
    return this->Build_pimpl_state_.Build_;
  }

  // Project_pimpl
  //

  Project_pimpl::
  Project_pimpl (bool b)
  {
    this->Project_pimpl_base_ = b;
    this->Project_pimpl_state_.Project_ = 0;
  }

  Project_pimpl::
  ~Project_pimpl ()
  {
    if (!this->Project_pimpl_base_ && this->Project_pimpl_state_.Project_)
      delete this->Project_pimpl_state_.Project_;
  }

  void Project_pimpl::
  _reset ()
  {
    Project_pskel::_reset ();

    if (!this->Project_pimpl_base_ && this->Project_pimpl_state_.Project_)
    {
      delete this->Project_pimpl_state_.Project_;
      this->Project_pimpl_state_.Project_ = 0;
    }
  }

  void Project_pimpl::
  pre_impl (::megaxml::Project* x)
  {
    this->Project_pimpl_state_.Project_ = x;
  }

  void Project_pimpl::
  pre ()
  {
    ::megaxml::Project* x = new ::megaxml::Project;
    this->pre_impl (x);
  }

  void Project_pimpl::
  Name (const ::std::string& x)
  {
    this->Project_pimpl_state_.Project_->Name (x);
  }

  void Project_pimpl::
  Host (const ::std::string& x)
  {
    this->Project_pimpl_state_.Project_->Host (x);
  }

  void Project_pimpl::
  Description (const ::std::string& x)
  {
    this->Project_pimpl_state_.Project_->Description (x);
  }

  void Project_pimpl::
  Package (const ::std::string& x)
  {
    this->Project_pimpl_state_.Project_->Package ().push_back (x);
  }

  void Project_pimpl::
  Files (::megaxml::Files* x)
  {
    this->Project_pimpl_state_.Project_->Files (x);
  }

  void Project_pimpl::
  Build (const ::megaxml::Build& x)
  {
    this->Project_pimpl_state_.Project_->Build ().push_back (x);
  }

  void Project_pimpl::
  Run (::megaxml::Run* x)
  {
    this->Project_pimpl_state_.Project_->Run ().push_back (x);
  }

  void Project_pimpl::
  Defaults (const ::megaxml::Defaults& x)
  {
    this->Project_pimpl_state_.Project_->Defaults (x);
  }

  ::megaxml::Project* Project_pimpl::
  post_Project ()
  {
    ::megaxml::Project* r = this->Project_pimpl_state_.Project_;
    this->Project_pimpl_state_.Project_ = 0;
    return r;
  }

  // Files_pimpl
  //

  Files_pimpl::
  Files_pimpl (bool b)
  {
    this->Files_pimpl_base_ = b;
    this->Files_pimpl_state_.Files_ = 0;
  }

  Files_pimpl::
  ~Files_pimpl ()
  {
    if (!this->Files_pimpl_base_ && this->Files_pimpl_state_.Files_)
      delete this->Files_pimpl_state_.Files_;
  }

  void Files_pimpl::
  _reset ()
  {
    Files_pskel::_reset ();

    if (!this->Files_pimpl_base_ && this->Files_pimpl_state_.Files_)
    {
      delete this->Files_pimpl_state_.Files_;
      this->Files_pimpl_state_.Files_ = 0;
    }
  }

  void Files_pimpl::
  pre_impl (::megaxml::Files* x)
  {
    this->Files_pimpl_state_.Files_ = x;
  }

  void Files_pimpl::
  pre ()
  {
    ::megaxml::Files* x = new ::megaxml::Files;
    this->pre_impl (x);
  }

  void Files_pimpl::
  System (const ::std::string& x)
  {
    this->Files_pimpl_state_.Files_->System ().push_back (x);
  }

  void Files_pimpl::
  Include (const ::std::string& x)
  {
    this->Files_pimpl_state_.Files_->Include ().push_back (x);
  }

  void Files_pimpl::
  Source (const ::std::string& x)
  {
    this->Files_pimpl_state_.Files_->Source ().push_back (x);
  }

  void Files_pimpl::
  Library (const ::std::string& x)
  {
    this->Files_pimpl_state_.Files_->Library ().push_back (x);
  }

  ::megaxml::Files* Files_pimpl::
  post_Files ()
  {
    ::megaxml::Files* r = this->Files_pimpl_state_.Files_;
    this->Files_pimpl_state_.Files_ = 0;
    return r;
  }

  // EG_pimpl
  //

  EG_pimpl::
  EG_pimpl (bool b)
  {
    this->EG_pimpl_base_ = b;
    this->EG_pimpl_state_.EG_ = 0;
  }

  EG_pimpl::
  ~EG_pimpl ()
  {
    if (!this->EG_pimpl_base_ && this->EG_pimpl_state_.EG_)
      delete this->EG_pimpl_state_.EG_;
  }

  void EG_pimpl::
  _reset ()
  {
    EG_pskel::_reset ();

    if (!this->EG_pimpl_base_ && this->EG_pimpl_state_.EG_)
    {
      delete this->EG_pimpl_state_.EG_;
      this->EG_pimpl_state_.EG_ = 0;
    }
  }

  void EG_pimpl::
  pre_impl (::megaxml::EG* x)
  {
    this->EG_pimpl_state_.EG_ = x;
  }

  void EG_pimpl::
  pre ()
  {
    ::megaxml::EG* x = new ::megaxml::EG;
    this->pre_impl (x);
  }

  void EG_pimpl::
  choice_arm (choice_arm_tag t)
  {
    this->EG_pimpl_state_.EG_->choice_arm (
      static_cast< ::megaxml::EG::choice_arm_tag > (t));
  }

  void EG_pimpl::
  Package (::megaxml::Package* x)
  {
    this->EG_pimpl_state_.EG_->Package (x);
  }

  void EG_pimpl::
  Host (::megaxml::Host* x)
  {
    this->EG_pimpl_state_.EG_->Host (x);
  }

  void EG_pimpl::
  Project (::megaxml::Project* x)
  {
    this->EG_pimpl_state_.EG_->Project (x);
  }

  ::megaxml::EG* EG_pimpl::
  post_EG ()
  {
    ::megaxml::EG* r = this->EG_pimpl_state_.EG_;
    this->EG_pimpl_state_.EG_ = 0;
    return r;
  }

  // Directories_pimpl
  //

  Directories_pimpl::
  Directories_pimpl (bool b)
  {
    this->Directories_pimpl_base_ = b;
    this->Directories_pimpl_state_.Directories_ = 0;
  }

  Directories_pimpl::
  ~Directories_pimpl ()
  {
    if (!this->Directories_pimpl_base_ && this->Directories_pimpl_state_.Directories_)
      delete this->Directories_pimpl_state_.Directories_;
  }

  void Directories_pimpl::
  _reset ()
  {
    Directories_pskel::_reset ();

    if (!this->Directories_pimpl_base_ && this->Directories_pimpl_state_.Directories_)
    {
      delete this->Directories_pimpl_state_.Directories_;
      this->Directories_pimpl_state_.Directories_ = 0;
    }
  }

  void Directories_pimpl::
  pre_impl (::megaxml::Directories* x)
  {
    this->Directories_pimpl_state_.Directories_ = x;
  }

  void Directories_pimpl::
  pre ()
  {
    ::megaxml::Directories* x = new ::megaxml::Directories;
    this->pre_impl (x);
  }

  void Directories_pimpl::
  Include (const ::std::string& x)
  {
    this->Directories_pimpl_state_.Directories_->Include ().push_back (x);
  }

  void Directories_pimpl::
  Library (const ::std::string& x)
  {
    this->Directories_pimpl_state_.Directories_->Library ().push_back (x);
  }

  ::megaxml::Directories* Directories_pimpl::
  post_Directories ()
  {
    ::megaxml::Directories* r = this->Directories_pimpl_state_.Directories_;
    this->Directories_pimpl_state_.Directories_ = 0;
    return r;
  }

  // Run_pimpl
  //

  Run_pimpl::
  Run_pimpl (bool b)
  {
    this->Run_pimpl_base_ = b;
    this->Run_pimpl_state_.Run_ = 0;
  }

  Run_pimpl::
  ~Run_pimpl ()
  {
    if (!this->Run_pimpl_base_ && this->Run_pimpl_state_.Run_)
      delete this->Run_pimpl_state_.Run_;
  }

  void Run_pimpl::
  _reset ()
  {
    Run_pskel::_reset ();

    if (!this->Run_pimpl_base_ && this->Run_pimpl_state_.Run_)
    {
      delete this->Run_pimpl_state_.Run_;
      this->Run_pimpl_state_.Run_ = 0;
    }
  }

  void Run_pimpl::
  pre_impl (::megaxml::Run* x)
  {
    this->Run_pimpl_state_.Run_ = x;
  }

  void Run_pimpl::
  pre ()
  {
    ::megaxml::Run* x = new ::megaxml::Run;
    this->pre_impl (x);
  }

  void Run_pimpl::
  Name (const ::std::string& x)
  {
    this->Run_pimpl_state_.Run_->Name (x);
  }

  void Run_pimpl::
  Command (const ::std::string& x)
  {
    this->Run_pimpl_state_.Run_->Command (x);
  }

  void Run_pimpl::
  Argument (const ::std::string& x)
  {
    this->Run_pimpl_state_.Run_->Argument ().push_back (x);
  }

  ::megaxml::Run* Run_pimpl::
  post_Run ()
  {
    ::megaxml::Run* r = this->Run_pimpl_state_.Run_;
    this->Run_pimpl_state_.Run_ = 0;
    return r;
  }
}

namespace megaxml
{
  // EG_paggr
  //

  EG_paggr::
  EG_paggr ()
  {
    this->Defaults_p_.parsers (this->Fibers_p_);

    this->Fibers_p_.parsers (this->Stack_p_);

    this->Stack_p_.parsers (this->unsigned_int_p_);

    this->Files_p_.parsers (this->string_p_,
                            this->string_p_,
                            this->string_p_,
                            this->string_p_);

    this->EG_p_.parsers (this->Package_p_,
                         this->Host_p_,
                         this->Project_p_);

    this->Package_p_.parsers (this->string_p_,
                              this->string_p_,
                              this->string_p_,
                              this->string_p_,
                              this->Directories_p_,
                              this->Files_p_,
                              this->string_p_);

    this->Directories_p_.parsers (this->string_p_,
                                  this->string_p_);

    this->Host_p_.parsers (this->string_p_,
                           this->string_p_,
                           this->string_p_,
                           this->string_p_,
                           this->Directories_p_,
                           this->Files_p_,
                           this->string_p_);

    this->Project_p_.parsers (this->string_p_,
                              this->string_p_,
                              this->string_p_,
                              this->string_p_,
                              this->Files_p_,
                              this->Build_p_,
                              this->Run_p_,
                              this->Defaults_p_);

    this->Build_p_.parsers (this->string_p_,
                            this->string_p_,
                            this->string_p_);

    this->Run_p_.parsers (this->string_p_,
                          this->string_p_,
                          this->string_p_);
  }

  const char* EG_paggr::
  root_name ()
  {
    return "EG";
  }

  const char* EG_paggr::
  root_namespace ()
  {
    return "";
  }
}

// Begin epilogue.
//
//
// End epilogue.

