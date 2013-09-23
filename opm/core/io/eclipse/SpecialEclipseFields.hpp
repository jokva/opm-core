//===========================================================================
//
// File: SpecialEclipseFields.hpp
//
// Created: Mon Sep 21 14:09:54 2009
//
// Author(s): Atgeirr F Rasmussen <atgeirr@sintef.no>
//            Bård Skaflestad     <bard.skaflestad@sintef.no>
//            Bjørn Spjelkavik    <bsp@sintef.no>
//
// $Date$
//
// $Revision$
//
//===========================================================================

/*
  Copyright 2009, 2010 SINTEF ICT, Applied Mathematics.
  Copyright 2009, 2010 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPM_SPECIALECLIPSEFIELDS_HEADER
#define OPM_SPECIALECLIPSEFIELDS_HEADER

#include <opm/core/utility/ErrorMacros.hpp>
#include <opm/core/io/eclipse/EclipseGridParserHelpers.hpp>
#include <opm/core/io/eclipse/EclipseUnits.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <limits>

namespace Opm
{

// Abstract base class for special fields.
struct SpecialBase {
    virtual ~SpecialBase() {}                       // Default destructor
    //virtual std::string name() const = 0;           // Keyword name
    virtual void read(std::istream& is) = 0;        // Reads data
    //virtual void write(std::ostream& os) const = 0; // Writes data
    virtual void convertToSI(const EclipseUnits&)
    {
        OPM_THROW(std::runtime_error, "Default conversion not defined.");
    }
    typedef std::vector<std::vector<std::vector<double> > > table_t;
};




/// Class for keyword SPECGRID
struct SPECGRID : public SpecialBase
{
    std::vector<int> dimensions; // Number of grid blocks in x-, y- and z-directions.
    int numres;          // Number of reservoirs.
    char qrdial;         // Coordinates. F=cartesian, T=Cylindrical(radial).

    SPECGRID()
    {
        dimensions.resize(3,1);
        numres = 1;
        qrdial = 'F';
    }

    virtual ~SPECGRID()
    {
    }

    virtual std::string name() const {return std::string("SPECGRID");}

    virtual void read(std::istream& is)
    {
        const int ndim = 3;
        std::vector<int> data(ndim+1,1);
        int nread = readDefaultedVectorData(is , data, ndim+1);
        int nd = std::min(nread, ndim);
        copy(data.begin(), data.begin()+nd, &dimensions[0]);
        numres = data[ndim];
        std::string candidate;
        is >> candidate;
        if (candidate == "/") {
            return;
        } else {
            qrdial = candidate[0];
        }

        if (ignoreSlashLine(is)) {
            return;
        } else {
            OPM_THROW(std::runtime_error, "End of file reading" << name());
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        os << dimensions[0] << " " << dimensions[1]  << " "
           << dimensions[2] << " " << numres << " " << qrdial << std::endl;
        os << std::endl;
    }

    virtual void convertToSI(const EclipseUnits&)
    {}
};




/// Class holding segment data of keyword FAULTS.
struct FaultSegment
{
    std::string fault_name;          // Fault name
    std::vector<int> ijk_coord;      // ijk-coordinates of segment cells
    std::string face;                // Fault face of cells
};

/// Class for keyword FAULTS.
struct FAULTS : public SpecialBase
{
    std::vector<FaultSegment> faults;

    FAULTS()
    {
    }

    virtual ~FAULTS()
    {
    }

    virtual std::string name() const {return std::string("FAULTS");}

    virtual void read(std::istream& is)
    {
        while(is) {
            std::string fltname;
            is >> fltname;
            if (fltname[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (fltname.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine >> fltname;
            }
            FaultSegment fault_segment;
            fault_segment.ijk_coord.resize(6);
            fault_segment.fault_name = fltname;
            int nread = readDefaultedVectorData(is, fault_segment.ijk_coord, 6);
            if (nread != 6) {
                OPM_THROW(std::runtime_error, "Error reading fault_segment " << fltname);
            }
            is >> fault_segment.face;
            faults.push_back(fault_segment);
            ignoreSlashLine(is);
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        for (int i=0; i<(int)faults.size(); ++i) {
            os << faults[i].fault_name << "  ";
            copy(faults[i].ijk_coord.begin(), faults[i].ijk_coord.end(),
                 std::ostream_iterator<int>(os, " "));
            os << faults[i].face << std::endl;
        }
        os << std::endl;
    }

    virtual void convertToSI(const EclipseUnits&)
    {}
};




/// Class holding a data line of keyword MULTFLT
struct MultfltLine
{
    std::string fault_name;         // Fault name, as in FAULTS
    double transmis_multiplier;     // Transmissibility multiplier
    double diffusivity_multiplier;  // Diffusivity multiplier;
    MultfltLine() :
        fault_name(""), transmis_multiplier(1.0), diffusivity_multiplier(1.0)
    {}
};

/// Class for keyword MULFLT
struct MULTFLT : public SpecialBase
{
    std::vector<MultfltLine> multflts;

    MULTFLT()
    {
    }

    virtual ~MULTFLT()
    {}

    virtual std::string name() const {return std::string("MULTFLT");}

    virtual void read(std::istream& is)
    {
        while(is) {
            std::string fltname;
            is >> fltname;
            if (fltname[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (fltname == "--") {
                // This line is a comment
                is >> ignoreLine >> fltname;
            }
            MultfltLine multflt_line;
            multflt_line.fault_name = fltname;
            std::vector<double> data(2,1.0);
            if (readDefaultedVectorData(is, data, 2) == 2) {
                ignoreSlashLine(is);
            }
            multflt_line.transmis_multiplier = data[0];
            multflt_line.diffusivity_multiplier = data[1];
            multflts.push_back(multflt_line);
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        for (int i=0; i<(int)multflts.size(); ++i) {
            os << multflts[i].fault_name << "  "
               << multflts[i].transmis_multiplier << "  "
               << multflts[i].diffusivity_multiplier <<        std::endl;
        }
        os << std::endl;
    }

    virtual void convertToSI(const EclipseUnits&)
    {}
};




struct TITLE : public SpecialBase
{
    std::string title;
    virtual std::string name() const
    { return std::string("TITLE"); }
    virtual void read(std::istream& is)
    { is >> ignoreLine; std::getline(is, title); }
    virtual void write(std::ostream& os) const
    { os << name() << '\n' << title << '\n'; }
    virtual void convertToSI(const EclipseUnits&)
    {}
};




struct START : public SpecialBase
{
    boost::gregorian::date date;
    virtual std::string name() const
    { return std::string("START"); }
    virtual void read(std::istream& is)
    { date = readDate(is); }
    virtual void write(std::ostream& os) const
    { os << name() << '\n' << date << '\n'; }
    virtual void convertToSI(const EclipseUnits&)
    {}
};

struct GRUPTREE : public SpecialBase {
    std::map<std::string, std::string> tree;

    virtual std::string name() const {
        return std::string("GRUPTREE");
    }

    virtual void read(std::istream & is) {
        while (!is.eof()) {
            std::string child = readString(is);
            is >> ignoreWhitespace;
            std::string parent = readString(is);

            std::cout << "CHILD = " << child << std::endl << "PARENT = " << parent << std::endl;
            tree[child] = parent;

            is >> ignoreSlashLine;
            is >> ignoreWhitespace;
            if(is.peek() == int('/')) {
                is >> ignoreSlashLine;
                return;
            }
        }
    }

    virtual void write(std::ostream & os) {
        for(std::map<std::string, std::string>::iterator it = tree.begin(); it != tree.end(); ++it) {
            os << it->first << " "<< it->second<< std::endl;
        }
    }

    virtual void convertToSI(const EclipseUnits&) {
    }
};


struct DATES : public SpecialBase
{
    std::vector<boost::gregorian::date> dates;
    virtual std::string name() const
    { return std::string("DATES"); }
    virtual void read(std::istream& is)
    {
        while(is) {
            dates.push_back(readDate(is));
            is >> ignoreWhitespace;
            if (is.peek() == int('/')) {
                is >> ignoreLine;
                break;
            }
        }
    }
    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        copy(dates.begin(), dates.end(),
             std::ostream_iterator<boost::gregorian::date>(os, "\n"));
    }
    virtual void convertToSI(const EclipseUnits&)
    {}
};


struct DENSITY : public SpecialBase
{
    std::vector<std::vector<double> > densities_;

    virtual std::string name() const {return std::string("DENSITY");}

    virtual void read(std::istream& is)
    {
        while (!is.eof()) {
            std::vector<double> density(3,-1e100);
            if (readDefaultedVectorData(is, density, 3) == 3) {
                ignoreSlashLine(is);
            }
            densities_.push_back(density);

            int action = next_action(is); // 0:continue  1:return  2:throw
            if (action == 1) {
                return;     // Alphabetic char. Read next keyword.
            } else if (action == 2) {
                OPM_THROW(std::runtime_error, "Error reading DENSITY. Next character is "
                      << (char)is.peek());
            }
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int i=0; i<(int)densities_.size(); ++i) {
            os << densities_[i][0] << " " << densities_[i][1] << " "
               << densities_[i][2] << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int i=0; i<(int)densities_.size(); ++i) {
            densities_[i][0] *= units.density;
            densities_[i][1] *= units.density;
            densities_[i][2] *= units.density;
        }
    }
};

struct PVDG : public SpecialBase
{
    table_t pvdg_;

    virtual std::string name() const {return std::string("PVDG");}

    virtual void read(std::istream& is)
    {
        readPvdTable(is, pvdg_, name(), 3);
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int rn=0; rn<(int)pvdg_.size(); ++rn) {
            for (int i=0; i<(int)pvdg_[rn][0].size(); ++i) {
                os << pvdg_[rn][0][i] << " " << pvdg_[rn][1][i] << " "
                   << pvdg_[rn][2][i] << '\n';
            }
            os << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        double volfac = units.gasvol_r/units.gasvol_s;
        for (int rn=0; rn<(int)pvdg_.size(); ++rn) {
            for (int i=0; i<(int)pvdg_[rn][0].size(); ++i) {
                pvdg_[rn][0][i] *= units.pressure;
                pvdg_[rn][1][i] *= volfac;
                pvdg_[rn][2][i] *= units.viscosity;
            }
        }
    }
};

struct PVDO : public SpecialBase
{
    table_t pvdo_;

    virtual std::string name() const {return std::string("PVDO");}

    virtual void read(std::istream& is)
    {
        readPvdTable(is, pvdo_, name(), 3);
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int rn=0; rn<(int)pvdo_.size(); ++rn) {
            for (int i=0; i<(int)pvdo_[rn][0].size(); ++i) {
                os << pvdo_[rn][0][i] << " " << pvdo_[rn][1][i] << " "
                   << pvdo_[rn][2][i] << '\n';
            }
            os << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        double volfac = units.liqvol_r/units.liqvol_s;
        for (int rn=0; rn<(int)pvdo_.size(); ++rn) {
            for (int i=0; i<(int)pvdo_[rn][0].size(); ++i) {
                pvdo_[rn][0][i] *= units.pressure;
                pvdo_[rn][1][i] *= volfac;
                pvdo_[rn][2][i] *= units.viscosity;
            }
        }
    }
};

struct PVTG : public SpecialBase
{
    table_t pvtg_;

    virtual std::string name() const {return std::string("PVTG");}

    virtual void read(std::istream& is)
    {
        readPvtTable(is, pvtg_, name());
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int rn=0; rn<(int)pvtg_.size(); ++rn) {
            for (int i=0; i<(int)pvtg_[rn].size(); ++i) {
                int nl = (pvtg_[rn][i].size()-1) / 3;
                os << pvtg_[rn][i][0] << "   " << pvtg_[rn][i][1] << "  "
                   << pvtg_[rn][i][2] << "  " << pvtg_[rn][i][3] <<  '\n';
                for (int j=1, n=3; j<nl; ++j, n+=3) {
                    os << '\t' << pvtg_[rn][i][n+1] << "  "
                       << pvtg_[rn][i][n+2] << "  " << pvtg_[rn][i][n+3]
                       <<  '\n';
                }
            }
            os << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        double oilgasratio = units.liqvol_s/units.gasvol_s;
        double volfac = units.gasvol_r/units.gasvol_s;
        for (int rn=0; rn<(int)pvtg_.size(); ++rn) {
            for (int i=0; i<(int)pvtg_[rn].size(); ++i) {
                int nl = (pvtg_[rn][i].size()-1) / 3;
                pvtg_[rn][i][0] *= units.pressure;
                pvtg_[rn][i][1] *= oilgasratio;
                pvtg_[rn][i][2] *= volfac;
                pvtg_[rn][i][3] *= units.viscosity;
                for (int j=1, n=3; j<nl; ++j, n+=3) {
                    pvtg_[rn][i][n+1] *= oilgasratio;
                    pvtg_[rn][i][n+2] *= volfac;
                    pvtg_[rn][i][n+3] *= units.viscosity;
                }
            }
        }
    }
};


struct PVTO : public SpecialBase
{
    table_t pvto_;

    virtual std::string name() const {return std::string("PVTO");}

    virtual void read(std::istream& is)
    {
        readPvtTable(is, pvto_, name());
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int rn=0; rn<(int)pvto_.size(); ++rn) {
            for (int i=0; i<(int)pvto_[rn].size(); ++i) {
                int nl = (pvto_[rn][i].size()-1) / 3;
                os << pvto_[rn][i][0] << "   " << pvto_[rn][i][1] << "  "
                   << pvto_[rn][i][2] << "  " << pvto_[rn][i][3] <<  '\n';
                for (int j=1, n=3; j<nl; ++j, n+=3) {
                    os << '\t' << pvto_[rn][i][n+1] << "  "
                       << pvto_[rn][i][n+2] << "  " << pvto_[rn][i][n+3]
                       <<  '\n';
                }
            }
            os << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        double gasoilratio = units.gasvol_s/units.liqvol_s;
        double volfac = units.liqvol_r/units.liqvol_s;
        for (int rn=0; rn<(int)pvto_.size(); ++rn) {
            for (int i=0; i<(int)pvto_[rn].size(); ++i) {
                int nl = (pvto_[rn][i].size()-1) / 3;
                pvto_[rn][i][0] *= gasoilratio;
                pvto_[rn][i][1] *= units.pressure;
                pvto_[rn][i][2] *= volfac;
                pvto_[rn][i][3] *= units.viscosity;
                for (int j=1, n=3; j<nl; ++j, n+=3) {
                    pvto_[rn][i][n+1] *= units.pressure;
                    pvto_[rn][i][n+2] *= volfac;
                    pvto_[rn][i][n+3] *= units.viscosity;
                }
            }
        }
    }
};


struct PVTW : public SpecialBase
{
    std::vector<std::vector<double> > pvtw_;

    virtual std::string name() const {return std::string("PVTW");}

    virtual void read(std::istream& is)
    {
        while (!is.eof()) {
            std::vector<double> pvtw;
            readVectorData(is, pvtw);
            if (pvtw.size() == 4) {
                pvtw.push_back(0.0); // Not used by frontsim
            }
            pvtw_.push_back(pvtw);

            int action = next_action(is); // 0:continue  1:return  2:throw
            if (action == 1) {
                return;     // Alphabetic char. Read next keyword.
            } else if (action == 2) {
                OPM_THROW(std::runtime_error, "Error reading PVTW. Next character is "
                      <<  (char)is.peek());
            }
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int i=0; i<(int)pvtw_.size(); ++i) {
            os << pvtw_[i][0] << " " << pvtw_[i][1] << " " << pvtw_[i][2]
               << " " << pvtw_[i][3] << " " << pvtw_[i][4] << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        double volfac = units.liqvol_r/units.liqvol_s;
        for (int i=0; i<(int)pvtw_.size(); ++i) {
            pvtw_[i][0] *= units.pressure;
            pvtw_[i][1] *= volfac;
            pvtw_[i][2] *= units.compressibility;
            pvtw_[i][3] *= units.viscosity;
            pvtw_[i][4] *= units.compressibility;
        }
    }
};


struct ROCK : public SpecialBase
{
    std::vector<std::vector<double> > rock_compressibilities_;

    virtual std::string name() const {return std::string("ROCK");}

    virtual void read(std::istream& is)
    {
        while (!is.eof()) {
            std::vector<double> rock;
            readVectorData(is, rock);
            rock_compressibilities_.push_back(rock);

            int action = next_action(is); // 0:continue  1:return  2:throw
            if (action == 1) {
                return;     // Alphabetic char. Read next keyword.
            } else if (action == 2) {
                OPM_THROW(std::runtime_error, "Error reading ROCK. Next character is "
                      << (char)is.peek());
            }
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int i=0; i<(int)rock_compressibilities_.size(); ++i) {
            os << rock_compressibilities_[i][0] << " "
               << rock_compressibilities_[i][1] << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int i=0; i<(int)rock_compressibilities_.size(); ++i) {
            rock_compressibilities_[i][0] *= units.pressure;
            rock_compressibilities_[i][1] *= units.compressibility;
        }
    }
};


struct ROCKTAB : public SpecialBase
{
    table_t rocktab_;

    virtual std::string name() const {return std::string("ROCKTAB");}

    virtual void read(std::istream& is)
    {
        readPvdTable(is, rocktab_, name(), 3);
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int rn=0; rn<(int)rocktab_.size(); ++rn) {
            for (int i=0; i<(int)rocktab_[rn][0].size(); ++i) {
                os << rocktab_[rn][0][i] << " " << rocktab_[rn][1][i] << " "
                   << rocktab_[rn][2][i] << '\n';
            }
            os << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int rn=0; rn<(int)rocktab_.size(); ++rn) {
             for (int i=0; i<(int)rocktab_[rn][0].size(); ++i) {
                rocktab_[rn][0][i] *= units.pressure;
            }
        }
    }
};


struct SGOF : public SpecialBase
{
    table_t sgof_;

    virtual std::string name() const {return std::string("SGOF");}

    virtual void read(std::istream& is) {readSGWOF(is, sgof_, name(), 4);}

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int rn=0; rn<(int)sgof_.size(); ++rn) {
            for (int i=0; i<(int)sgof_[rn][0].size(); ++i) {
                os << sgof_[rn][0][i] << " " << sgof_[rn][1][i] << " "
                   << sgof_[rn][2][i] << " " << sgof_[rn][3][i] << '\n';
            }
            os << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int rn=0; rn<(int)sgof_.size(); ++rn) {
            for (int i=0; i<(int)sgof_[rn][0].size(); ++i) {
                sgof_[rn][3][i] *= units.pressure;
            }
        }
    }
};

struct SWOF : public SpecialBase
{
    table_t swof_;

    virtual std::string name() const {return std::string("SWOF");}

    virtual void read(std::istream& is) {readSGWOF(is, swof_, name(), 4);}

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int rn=0; rn<(int)swof_.size(); ++rn) {
            for (int i=0; i<(int)swof_[rn][0].size(); ++i) {
                os << swof_[rn][0][i] << " " << swof_[rn][1][i] << " "
                   << swof_[rn][2][i] << " " << swof_[rn][3][i] << '\n';
            }
            os << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int rn=0; rn<(int)swof_.size(); ++rn) {
            for (int i=0; i<(int)swof_[rn][0].size(); ++i) {
                swof_[rn][3][i] *= units.pressure;
            }
        }
    }
};

/// Class holding a data line of keyword WELSPECS
struct WelspecsLine
{
    std::string name_;               // Well name
    std::string group_;              // Group name
    int I_;                          // I-location of well head or heel
    int J_;                          // J-location of well head or heel
    double datum_depth_BHP_;         // Datum depth for bottom hole pressure
    std::string pref_phase_;         // Preferred phase for the well
    double drain_rad_;               // Drainage radius for prod/inj index calculation
    std::string spec_inflow_;        // Flag for special inflow equation
    std::string shut_in_;            // Instructions for automatic shut-in
    std::string crossflow_;          // Crossflow ability flag
    int pressure_table_number_;      // Pressure table number for wellbore fluid properties
    std::string density_calc_type_;  // Type of density calculation for wellbore hydrostatic head
    int fluids_in_place_reg_numb_;   // Fluids in place region number

    WelspecsLine() :
        name_(), group_(), I_(-1), J_(-1),
        datum_depth_BHP_(-1.0), pref_phase_(), drain_rad_(0.0),
        spec_inflow_("STD"), shut_in_("SHUT"), crossflow_("YES"),
        pressure_table_number_(0), density_calc_type_("SEG"),
        fluids_in_place_reg_numb_(0)
    {}
};

/// Class for keyword WELSPECS
struct WELSPECS : public SpecialBase
{
    std::vector<WelspecsLine> welspecs;

    WELSPECS()
    {
    }

    virtual ~WELSPECS()
    {}

    virtual std::string name() const {return std::string("WELSPECS");}

    virtual void read(std::istream& is)
    {
        while(is) {
            std::string wellname = readString(is);
            if (wellname[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (wellname.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                wellname = readString(is);
            }
            WelspecsLine welspecs_line;
            welspecs_line.name_ = wellname;
            welspecs_line.group_ = readString(is);
            std::vector<int> int_data(2,1);
            readDefaultedVectorData(is, int_data, 2);
            welspecs_line.I_ = int_data[0];
            welspecs_line.J_ = int_data[1];
            std::vector<double> double_data(1,-1.0);
            readDefaultedVectorData(is, double_data, 1);
            welspecs_line.datum_depth_BHP_ = double_data[0];
            welspecs_line.pref_phase_ = readString(is);

            // HACK! Ignore items 7-13.
            ignoreSlashLine(is);
            welspecs.push_back(welspecs_line);

            // double_data[0] = 0.0;
            // readDefaultedVectorData(is, double_data, 1);
            // welspecs_line.drain_rad_ = double_data[0];
            // welspecs_line.spec_inflow_ = readString(is);
            // welspecs_line.shut_in_ = readString(is);
            // welspecs_line.crossflow_ = readString(is);
            // int_data[0] = 0;
            // readDefaultedVectorData(is, int_data, 1);
            // welspecs_line.pressure_table_number_ = int_data[0];
            // welspecs_line.density_calc_type_ = readString(is);
            // int_data[0] = 0;
            // readDefaultedVectorData(is, int_data, 1);
            // welspecs_line.fluids_in_place_reg_numb_ = int_data[0];
            // welspecs.push_back(welspecs_line);
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        for (int i=0; i<(int)welspecs.size(); ++i) {
            os << welspecs[i].name_ << "  "
               << welspecs[i].group_ << "  "
               << welspecs[i].I_ << "  "
               << welspecs[i].J_ << "  "
               << welspecs[i].datum_depth_BHP_ << "  "
               << welspecs[i].pref_phase_ << "  "
               << welspecs[i].drain_rad_ << "  "
               << welspecs[i].spec_inflow_ << "  "
               << welspecs[i].shut_in_ << "  "
               << welspecs[i].crossflow_ << "  "
               << welspecs[i].pressure_table_number_ << "  "
               << welspecs[i].density_calc_type_ << "  "
               << welspecs[i].fluids_in_place_reg_numb_ << "  "
               << "/\n";
        }
        os << '/' << std::endl;
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int i=0; i<(int)welspecs.size(); ++i) {
            welspecs[i].datum_depth_BHP_ *= units.length;
            welspecs[i].drain_rad_ *= units.length;
        }
    }
};

/// Class holding a data line of keyword COMPDAT
struct CompdatLine
{
    std::string well_;               // Well name
    std::vector<int> grid_ind_;      // Grid block location
    std::string open_shut_flag_;     // Open/shut flag of connection
    int sat_table_number_;           // Saturation table number
    double connect_trans_fac_;       // Connection transmillibilty factor
    double diameter_;                // Well bore internal diameter
    double Kh_;                      // Effective Kh value of the connection
    double skin_factor_;             // Skin factor
    double D_factor_;                // D-factor, for non-Darcy flow of free gas
    std::string penetration_direct_; // Penetration direction
    double r0_;                      // Pressure equivalent radius

    // Default values
    CompdatLine() :
        open_shut_flag_("OPEN"),
        sat_table_number_(0),
        connect_trans_fac_(0.0),
        diameter_(0.0),
        Kh_(-1.0),
        skin_factor_(0.0),
        D_factor_(-1e100),
        penetration_direct_("Z"),
        r0_(-1.0)
    {
        grid_ind_.resize(4);
    }
};

/// Class for keyword COMPDAT
struct COMPDAT : public SpecialBase
{
    std::vector<CompdatLine> compdat;

    COMPDAT()
    {
    }

    virtual ~COMPDAT()
    {}

    virtual std::string name() const {return std::string("COMPDAT");}

    virtual void read(std::istream& is)
    {
        while(is) {
            std::string wellname = readString(is);
            if (wellname[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (wellname.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                wellname = readString(is);
            }
            CompdatLine compdat_line;
            compdat_line.well_ = wellname;
            readDefaultedVectorData(is, compdat_line.grid_ind_, 4);
            compdat_line.open_shut_flag_ = readString(is);
            std::vector<int> int_data(1,-1);
            readDefaultedVectorData(is, int_data, 1);
            compdat_line.sat_table_number_ = int_data[0];
            std::vector<double> double_data(2, 0.0);
            int num_to_read = 2;
            int num_read = readDefaultedVectorData(is, double_data, num_to_read);
            compdat_line.connect_trans_fac_ = double_data[0];
            compdat_line.diameter_ = double_data[1];

            // HACK! Ignore items 10-14.
            if (num_read == num_to_read) {
                ignoreSlashLine(is);
            }
            compdat.push_back(compdat_line);
         }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        for (int i=0; i<(int)compdat.size(); ++i) {
            os << compdat[i].well_ << "  "
               << compdat[i].grid_ind_[0] << "  "
               << compdat[i].grid_ind_[1] << "  "
               << compdat[i].grid_ind_[2] << "  "
               << compdat[i].grid_ind_[3] << "  "
               << compdat[i].open_shut_flag_ << "  "
               << compdat[i].sat_table_number_ << "  "
               << compdat[i].connect_trans_fac_ << "  "
               << compdat[i].diameter_ << "  "
               << compdat[i].Kh_ << "  "
               << compdat[i].skin_factor_ << "  "
               << compdat[i].D_factor_ << "  "
               << compdat[i].penetration_direct_ << "  "
               << compdat[i].r0_ << "  "
               << "/\n";
        }
        os << '/' << std::endl;
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int i=0; i<(int)compdat.size(); ++i) {
            compdat[i].connect_trans_fac_ *= units.transmissibility;
            compdat[i].diameter_ *= units.length;
            compdat[i].Kh_ *= units.permeability*units.length;
            compdat[i].r0_ *= units.length;
        }
    }
};



/// Class holding a data line of keyword GCONINJE
struct GconinjeLine
{
    std::string group_;             // Well name or well name root
    std::string injector_type_;    // Injector type
    std::string control_mode_;     // Control mode
    double surface_flow_max_rate_; // Surface flow rate target or upper limit
    double resv_flow_max_rate_;    // Reservoir flow rate target or upper limit
    double reinjection_fraction_target_;
    double voidage_replacement_fraction_;

    // Default values
    GconinjeLine() :
        surface_flow_max_rate_(-1.0E20),
        resv_flow_max_rate_(-1E20),
        reinjection_fraction_target_(1),
        voidage_replacement_fraction_(1)
    {
    }
};

/// Class for keyword GCONINJE
struct GCONINJE : public SpecialBase
{
    std::vector<GconinjeLine> gconinje;

    GCONINJE()
    {
    }

    virtual ~GCONINJE()
    {}

    virtual std::string name() const {return std::string("GCONINJE");}

    virtual void read(std::istream& is)
    {
        while(is) {
            std::string groupname = readString(is);
            if (groupname[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (groupname.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                groupname = readString(is);
            }
            GconinjeLine gconinje_line;
            gconinje_line.group_ = groupname;
            gconinje_line.injector_type_ = readString(is);
            gconinje_line.control_mode_ = readString(is);
            std::vector<double> double_data(10, -1.0E20);
            const int num_to_read = 4;
            int num_read = readDefaultedVectorData(is, double_data, num_to_read);
            gconinje_line.surface_flow_max_rate_ = double_data[0];
            gconinje_line.resv_flow_max_rate_ = double_data[1];
            gconinje_line.reinjection_fraction_target_ = double_data[2];
            gconinje_line.voidage_replacement_fraction_ = double_data[3];
            // HACK! Ignore any further items
            if (num_read == num_to_read) {
                ignoreSlashLine(is);
            }
            gconinje.push_back(gconinje_line);

        }

    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        for (int i=0; i<(int) gconinje.size(); ++i) {
            os << gconinje[i].group_ << "  "
               << gconinje[i].injector_type_ << "  "
               << gconinje[i].control_mode_ << "  "
               << gconinje[i].surface_flow_max_rate_ << "  "
               << gconinje[i].reinjection_fraction_target_
               << std::endl;
        }
        os << std::endl;
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int i=0; i<(int) gconinje.size(); ++i) {
            if (gconinje[i].injector_type_ == "GAS") {
                gconinje[i].surface_flow_max_rate_ *= units.gasvol_s/units.time;
            } else {
                gconinje[i].surface_flow_max_rate_ *= units.liqvol_s/units.time;
            }
        }
    }
};




/// Class holding a data line of keyword WCONINJE
struct WconinjeLine
{
    std::string well_;             // Well name or well name root
    std::string injector_type_;    // Injector type
    std::string open_shut_flag_;   // Open/shut flag for the well
    std::string control_mode_;     // Control mode
    double surface_flow_max_rate_; // Surface flow rate target or upper limit
    double reservoir_flow_max_rate_; // Reservoir fluid volume rate target or
                                   // upper limit
    double BHP_limit_;             // BHP target or upper limit
    double THP_limit_;             // THP target or upper limit
    int VFP_table_number_;         // Injection well VFP table number
    double concentration_;         // Vaporised oil concentration in the
                                   // injected gas, or dissolved gas
                                   // concentration in the injected oil

    // Default values
    WconinjeLine() :
        open_shut_flag_("OPEN"), surface_flow_max_rate_(-1.0E20),
        reservoir_flow_max_rate_(-1.0E20), BHP_limit_(6891), THP_limit_(-1.0E20),
        VFP_table_number_(0), concentration_(0.0)
    {
    }
};

/// Class for keyword WCONINJE
struct WCONINJE : public SpecialBase
{
    std::vector<WconinjeLine> wconinje;

    WCONINJE()
    {
    }

    virtual ~WCONINJE()
    {}

    virtual std::string name() const {return std::string("WCONINJE");}

    virtual void read(std::istream& is)
    {
        while(is) {
            std::string wellname = readString(is);
            if (wellname[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (wellname.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                wellname = readString(is);
            }
            WconinjeLine wconinje_line;
            wconinje_line.well_ = wellname;
            wconinje_line.injector_type_ = readString(is);
            wconinje_line.open_shut_flag_ = readString(is);
            wconinje_line.control_mode_ = readString(is);
            std::vector<double> double_data(6, -1.0E20);
            double_data[2] = wconinje_line.BHP_limit_;
            double_data[4] = wconinje_line.VFP_table_number_;
            double_data[5] = wconinje_line.concentration_;
            const int num_to_read = 6;
            int num_read = readDefaultedVectorData(is, double_data, num_to_read);
            wconinje_line.surface_flow_max_rate_ = double_data[0];
            wconinje_line.reservoir_flow_max_rate_ = double_data[1];
            wconinje_line.BHP_limit_ = double_data[2];
            wconinje_line.THP_limit_ = double_data[3];
            wconinje_line.VFP_table_number_ = (int)double_data[4];
            wconinje_line.concentration_ = double_data[5];
            // HACK! Ignore any further items
            if (num_read == num_to_read) {
                ignoreSlashLine(is);
            }
            wconinje.push_back(wconinje_line);
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        for (int i=0; i<(int) wconinje.size(); ++i) {
            os << wconinje[i].well_ << "  "
               << wconinje[i].injector_type_ << "  "
               << wconinje[i].open_shut_flag_ << "  "
               << wconinje[i].control_mode_ << "  "
               << wconinje[i].surface_flow_max_rate_ << "  "
               << wconinje[i].reservoir_flow_max_rate_ << "  "
               << wconinje[i].BHP_limit_ << "  "
               << wconinje[i].THP_limit_ << "  "
               << wconinje[i].VFP_table_number_ << "  "
               << wconinje[i].concentration_
               << std::endl;
        }
        os << std::endl;
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int i=0; i<(int) wconinje.size(); ++i) {
            if (wconinje[i].injector_type_ == "GAS") {
                wconinje[i].surface_flow_max_rate_ *= units.gasvol_s/units.time;
            } else {
                wconinje[i].surface_flow_max_rate_ *= units.liqvol_s/units.time;
            }
            wconinje[i].reservoir_flow_max_rate_ *= units.liqvol_r/units.time;
            wconinje[i].BHP_limit_ *= units.pressure;
            wconinje[i].THP_limit_ *= units.pressure;
            wconinje[i].concentration_ *= units.gasvol_s/units.liqvol_s; // ??? @bsp 10
        }
    }
};



/// Class holding a data line of keyword GCONPROD
struct GconprodLine
{
    std::string group_;             // Well name or well name root
    std::string control_mode_;     // Control mode
    double oil_max_rate_;          // Oil rate target or upper limit
    double water_max_rate_;        // Water rate target or upper limit
    double gas_max_rate_;          // Gas rate target or upper limit
    double liquid_max_rate_;       // Liquid rate target or upper limit
    double resv_max_rate_;         // Reservoir liquid rate target or upper limit
    std::string procedure_;        // Procedure on exceeding a maximum rate limit
    // Default values
    GconprodLine() :
        oil_max_rate_(-1.0E20), water_max_rate_(-1.0E20),
        gas_max_rate_(-1.0E20), liquid_max_rate_(-1.0E20),
        resv_max_rate_(-1.0E20)
    {
    }
};

/// Class for keyword GCONPROD
struct GCONPROD : public SpecialBase
{
    std::vector<GconprodLine> gconprod;

    GCONPROD()
    {
    }

    virtual ~GCONPROD()
    {}

    virtual std::string name() const {return std::string("GCONPROD");}

    virtual void read(std::istream& is)
    {
        while(is) {
            std::string groupname = readString(is);
            if (groupname[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (groupname.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                groupname = readString(is);
            }
            GconprodLine gconprod_line;
            gconprod_line.group_ = groupname;
            gconprod_line.control_mode_ = readString(is);
            if (gconprod_line.control_mode_[gconprod_line.control_mode_.size() - 1] == '*')
            {
                gconprod_line.control_mode_ = "NONE";
            }
            std::vector<double> double_data(4, -1.0E20);
            const int num_to_read = 4;
            int num_read = readDefaultedVectorData(is, double_data, num_to_read);
            gconprod_line.oil_max_rate_ = double_data[0];
            gconprod_line.water_max_rate_ = double_data[1];
            gconprod_line.gas_max_rate_ = double_data[2];
            gconprod_line.liquid_max_rate_ = double_data[3];

            std::string procedure;
            if (num_read == num_to_read) {
                procedure = readString(is);
                if (procedure[0] == '/') {
                    is.putback('/');
                    procedure = "NONE";
                }
                int read_ignored = 0;
                int to_read = 6;
                for (; read_ignored < to_read; ++read_ignored) {
                    std::string ignored_value = readString(is);
                    if (ignored_value[ignored_value.size()-1]=='*') {
                        // we've got defaulted argument, increment

                        int num_defaulted;
                        std::string num_defaulted_str = ignored_value.substr(0, ignored_value.size()-1);
                        std::istringstream(num_defaulted_str) >> num_defaulted;
                        read_ignored += num_defaulted;
                        if (read_ignored >= to_read) {
                            break;
                        }
                    } else if(ignored_value[0] == '/') {
                        is.putback('/');
                        break;
                    }
                }
                if ( read_ignored <= to_read) {
                    // Not defaulted, we can read
                    std::string reservoir_volume = readString(is);
                    if (reservoir_volume[reservoir_volume.size()-1] == '*') {
                        // Defaulted, we're not doing anything
                    } else if(reservoir_volume[0] == '/') {
                        is.putback('/');
                    } else {
                        std::istringstream(reservoir_volume) >> gconprod_line.resv_max_rate_;
                    }
                }
            } else {
                procedure = "NONE";
            }
            gconprod_line.procedure_ = procedure;


            gconprod.push_back(gconprod_line);
            // HACK! Ignore any further items
            if (num_read == num_to_read) {
                ignoreSlashLine(is);
            }


        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        for (int i=0; i<(int) gconprod.size(); ++i) {
            os << gconprod[i].group_ << "  "
               << gconprod[i].control_mode_ << "  "
               << gconprod[i].oil_max_rate_ << "  "
               << gconprod[i].water_max_rate_ << "  "
               << gconprod[i].gas_max_rate_ << "  "
               << gconprod[i].liquid_max_rate_ << "  "
               << gconprod[i].procedure_ << " "
               << gconprod[i].resv_max_rate_
               << std::endl;
        }
        os << std::endl;
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        double lrat = units.liqvol_s / units.time;
        double grat = units.gasvol_s / units.time;
              double resv = units.liqvol_r / units.time;

        for (int i=0; i<(int) gconprod.size(); ++i) {
            gconprod[i].oil_max_rate_ *= lrat;
            gconprod[i].water_max_rate_ *= lrat;
            gconprod[i].gas_max_rate_ *= grat;
            gconprod[i].liquid_max_rate_ *= lrat;
            gconprod[i].resv_max_rate_ *= resv;
        }
    }
};

/// Class holding a data line of keyword WGRUPCON
struct WgrupconLine
{
    std::string well_;             // Well name
    bool available_for_group_control_;
    double guide_rate_;
    std::string phase_;
    WgrupconLine()
    : well_(), available_for_group_control_(true), guide_rate_(-1.0), phase_()
    {
    }
};

struct WGRUPCON : public SpecialBase
{
    std::vector<WgrupconLine> wgrupcon;

    WGRUPCON()
    {
    }

    virtual ~WGRUPCON()
    {}

    virtual std::string name() const {return std::string("WGRUPCON");}

    virtual void read(std::istream& is)
    {
        while(is) {
            std::string wname = readString(is);
            if (wname[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (wname.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                wname = readString(is);
            }
            WgrupconLine wgrupcon_line;
            wgrupcon_line.well_ = wname;
            std::string available = readString(is);
            if (available[available.size()-1] == '*')
            {
                available = "YES";
            }
            wgrupcon_line.available_for_group_control_ = (available == "YES");
            std::vector<double> double_data(1, 1.0E20);
            const int num_to_read = 1;
            int num_read = readDefaultedVectorData(is, double_data, num_to_read);
            wgrupcon_line.guide_rate_ = double_data[0];

            std::string phase = readString(is);
            if (phase[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (phase.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                phase = readString(is);
            }

            wgrupcon_line.phase_ = phase;

            wgrupcon.push_back(wgrupcon_line);
            // HACK! Ignore any further items
            if (num_read == num_to_read) {
                ignoreSlashLine(is);
            }

        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        for (int i=0; i<(int) wgrupcon.size(); ++i) {
            os << wgrupcon[i].well_ << "  "
               << wgrupcon[i].available_for_group_control_ << "  "
               << wgrupcon[i].guide_rate_ << "  "
               << wgrupcon[i].phase_ << "  "
               << std::endl;
        }
        os << std::endl;
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        static_cast<void>(units);
    }
};

/// Class holding a data line of keyword WCONPROD
struct WconprodLine
{
    std::string well_;             // Well name or well name root
    std::string open_shut_flag_;   // Open/shut flag for the well
    std::string control_mode_;     // Control mode
    double oil_max_rate_;          // Oil rate target or upper limit
    double water_max_rate_;        // Water rate target or upper limit
    double gas_max_rate_;          // Gas rate target or upper limit
    double liquid_max_rate_;       // Liquid rate target or upper limit
    double reservoir_flow_max_rate_; // Reservoir fluid volume rate target or
                                   // upper limit
    double BHP_limit_;             // BHP target or upper limit
    double THP_limit_;             // THP target or upper limit
    int VFP_table_number_;         // Injection well VFP table number
    double artif_lift_quantity_;   // Artificial lift quantity in THP calculations

    // Default values. Negative means no target or limit.
    // Note that the BHP default (1.0) will change meaning
    // depending on units used, but it is recommended practice
    // to not leave the BHP limit defaulted.
    WconprodLine() :
        open_shut_flag_("OPEN"), oil_max_rate_(-1.0E20), water_max_rate_(-1.0E20),
        gas_max_rate_(-1.0E20), liquid_max_rate_(-1.0E20),
        reservoir_flow_max_rate_(-1.0E20), BHP_limit_(1.0), THP_limit_(0.0),
        VFP_table_number_(0), artif_lift_quantity_(0.0)
    {
    }
};

/// Class for keyword WCONPROD
struct WCONPROD : public SpecialBase
{
    std::vector<WconprodLine> wconprod;

    WCONPROD()
    {
    }

    virtual ~WCONPROD()
    {}

    virtual std::string name() const {return std::string("WCONPROD");}

    virtual void read(std::istream& is)
    {
        while(is) {
            std::string wellname = readString(is);
            if (wellname[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (wellname.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                wellname = readString(is);
            }
            WconprodLine wconprod_line;
            wconprod_line.well_ = wellname;
            wconprod_line.open_shut_flag_ = readString(is);
            wconprod_line.control_mode_ = readString(is);
            std::vector<double> double_data(14, -1.0E20);
            double_data[5] = wconprod_line.BHP_limit_;
            double_data[6] = wconprod_line.THP_limit_;
            double_data[7] = wconprod_line.VFP_table_number_;
            double_data[8] = wconprod_line.artif_lift_quantity_;
            const int num_to_read = 14;
            int num_read = readDefaultedVectorData(is, double_data, num_to_read);
            wconprod_line.oil_max_rate_ = double_data[0];
            wconprod_line.water_max_rate_ = double_data[1];
            wconprod_line.gas_max_rate_ = double_data[2];
            wconprod_line.liquid_max_rate_ = double_data[3];
            wconprod_line.reservoir_flow_max_rate_ = double_data[4];
            wconprod_line.BHP_limit_ = double_data[5];
            wconprod_line.THP_limit_ = double_data[6];
            wconprod_line.VFP_table_number_ = (int)double_data[7];
            wconprod_line.artif_lift_quantity_ = double_data[8];
            wconprod.push_back(wconprod_line);

            if(num_to_read == num_read) {
                ignoreSlashLine(is);
            }

        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        for (int i=0; i<(int) wconprod.size(); ++i) {
            os << wconprod[i].well_ << "  "
               << wconprod[i].open_shut_flag_ << "  "
               << wconprod[i].control_mode_ << "  "
               << wconprod[i].oil_max_rate_ << "  "
               << wconprod[i].water_max_rate_ << "  "
               << wconprod[i].gas_max_rate_ << "  "
               << wconprod[i].liquid_max_rate_ << "  "
               << wconprod[i].reservoir_flow_max_rate_ << "  "
               << wconprod[i].BHP_limit_ << "  "
               << wconprod[i].THP_limit_ << "  "
               << wconprod[i].VFP_table_number_ << "  "
               << wconprod[i].artif_lift_quantity_
               << std::endl;
        }
        os << std::endl;
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        double lrat = units.liqvol_s / units.time;
        double grat = units.gasvol_s / units.time;
        double resv = units.liqvol_r / units.time;
        for (int i=0; i<(int) wconprod.size(); ++i) {
            wconprod[i].oil_max_rate_ *= lrat;
            wconprod[i].water_max_rate_ *= lrat;
            wconprod[i].gas_max_rate_ *= grat;
            wconprod[i].liquid_max_rate_ *= lrat;
            wconprod[i].reservoir_flow_max_rate_ *= resv;
            wconprod[i].BHP_limit_ *= units.pressure;
            wconprod[i].THP_limit_ *= units.pressure;
        }
    }
};


/// Class holding a data line of keyword WELTARG
struct WeltargLine
{
    std::string well_;             // Well name or well name root
    std::string control_change_;   // Definition of the control or constraint
                                   // quantity to be changed
    double new_value_;             // New value of this quantity

    WeltargLine()
    : well_(), control_change_(), new_value_(-1.0)
    {
    }
};

/// Class for keyword WELTARG
struct WELTARG : public SpecialBase
{
    std::vector<WeltargLine> weltarg;

    WELTARG()
    {
    }

    virtual ~WELTARG()
    {}

    virtual std::string name() const {return std::string("WELTARG");}

    virtual void read(std::istream& is)
    {
        while(is) {
            std::string wellname = readString(is);
            if (wellname[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (wellname.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                wellname = readString(is);
            }
            WeltargLine weltarg_line;
            weltarg_line.well_ = wellname;
            weltarg_line.control_change_ = readString(is);
            is >> weltarg_line.new_value_;
            ignoreSlashLine(is);
            weltarg.push_back(weltarg_line);
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        for (int i=0; i<(int)weltarg.size(); ++i) {
            os << weltarg[i].well_ << "  "
               << weltarg[i].control_change_ << "  "
               << weltarg[i].new_value_ << "  "
               << std::endl;
        }
        os << std::endl;
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        double lrat = units.liqvol_s / units.time;
        double grat = units.gasvol_s / units.time;
        double resv = units.liqvol_r / units.time;
        for (int i=0; i<(int) weltarg.size(); ++i) {
            if (weltarg[i].control_change_[0] == 'O' ||
                weltarg[i].control_change_[0] == 'W' ||
                weltarg[i].control_change_[0] == 'L') {
                weltarg[i].new_value_ *= lrat;
            } else if (weltarg[i].control_change_[0] == 'G') {
                weltarg[i].new_value_ *= grat;
            } else if (weltarg[i].control_change_[0] == 'R') {
                weltarg[i].new_value_ *= resv;
            } else if (weltarg[i].control_change_[0] == 'B' ||
                       weltarg[i].control_change_[0] == 'T') {
                weltarg[i].new_value_ *= units.pressure;
            } else {
                OPM_THROW(std::runtime_error, "WELTARG. Unknown control or constraint "
                      << weltarg[i].control_change_[0]);
            }
        }
    }
};



struct WelopenLine
{
    std::string well_;             // Well name or well name root
    std::string openshutflag_;     // What to do with the well.
};

/// Class for keyword WELOPEN
struct WELOPEN : public SpecialBase
{
    std::vector<WelopenLine> welopen;

    WELOPEN()
    {
    }

    virtual ~WELOPEN()
    {}

    virtual std::string name() const {return std::string("WELOPEN");}

    virtual void read(std::istream& is)
    {
        while(is) {
            std::string wellname = readString(is);
            if (wellname[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (wellname.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                wellname = readString(is);
            }
            WelopenLine welopen_line;
            welopen_line.well_ = wellname;
            welopen_line.openshutflag_ = readString(is);
            ignoreSlashLine(is);
            welopen.push_back(welopen_line);
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        for (int i=0; i<(int)welopen.size(); ++i) {
            os << welopen[i].well_ << "  "
               << welopen[i].openshutflag_
               << std::endl;
        }
        os << std::endl;
    }

    virtual void convertToSI(const EclipseUnits& /*units*/)
    {
    }
};




/// Class holding a data line of keyword EQUIL
struct EquilLine
{
    double datum_depth_;             // Datum depth
    double datum_depth_pressure_;    // Pressure at datum depth.
    double water_oil_contact_depth_; // Depth of water oil contact.
    double oil_water_cap_pressure_;  // Oil-water capillary pressure at the
                                     // water-oil contact or Gas-water capillary
                                     // pressure at the gas-water contact contact.
    double gas_oil_contact_depth_;   // Depth of the gas-oil contact.
    double gas_oil_cap_pressure_;    // Gas-oil capillary pressure at the gas-oil
                                     // contact.
    int live_oil_table_index_;       // Rs v Depth or Pb v Depth table index for
                                     // under-saturated live oil.
    int wet_gas_table_index_;        // Rv v Depth or Pd v Depth table index for
                                     // under-saturated wet gas.
    int N_;                          // Integer defining the accuracy of the
                                     // initial fluids in place calculation.
    EquilLine()
    {
        datum_depth_             = datum_depth_pressure_   = 0.0;
        water_oil_contact_depth_ = oil_water_cap_pressure_ = 0.0;
        gas_oil_contact_depth_   = gas_oil_cap_pressure_   = 0.0;

        live_oil_table_index_ = 0;
        wet_gas_table_index_  = 0;
        N_                    = 0;
    }
};

/// Class for keyword EQUIL
struct EQUIL : public SpecialBase
{
    std::vector<EquilLine> equil;

    EQUIL()
    {
    }

    virtual ~EQUIL()
    {}

    virtual std::string name() const {return std::string("EQUIL");}

    virtual void read(std::istream& is)
    {
        // Note. This function assumes that NTEQUL = 1, and reads only one line.
        int num_to_read = 9;
        std::vector<double> data(num_to_read,0);
        int num_read = readDefaultedVectorData(is, data, num_to_read);
        if (num_read == num_to_read) {
            ignoreSlashLine(is);
        }

        EquilLine equil_line;
        equil_line.datum_depth_             = data[0];
        equil_line.datum_depth_pressure_    = data[1];
        equil_line.water_oil_contact_depth_ = data[2];
        equil_line.oil_water_cap_pressure_  = data[3];
        equil_line.gas_oil_contact_depth_   = data[4];
        equil_line.gas_oil_cap_pressure_    = data[5];
        equil_line.live_oil_table_index_    = int(data[6]);
        equil_line.wet_gas_table_index_     = int(data[7]);
        equil_line.N_                       = int(data[8]);
        equil.push_back(equil_line);
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << std::endl;
        for (int i=0; i<(int)equil.size(); ++i) {
            os << equil[i].datum_depth_ << "  "
               << equil[i].datum_depth_pressure_ << "  "
               << equil[i].water_oil_contact_depth_ << "  "
               << equil[i].oil_water_cap_pressure_ << "  "
               << equil[i].gas_oil_contact_depth_ << "  "
               << equil[i].gas_oil_cap_pressure_ << "  "
               << equil[i].live_oil_table_index_ << "  "
               << equil[i].wet_gas_table_index_ << "  "
               << equil[i].N_ << "  "

               << std::endl;
        }
        os << std::endl;
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int i=0; i<(int)equil.size(); ++i) {
            equil[i].datum_depth_ *= units.length;
            equil[i].datum_depth_pressure_ *= units.pressure;
            equil[i].water_oil_contact_depth_ *= units.length;
            equil[i].oil_water_cap_pressure_ *= units.pressure;
            equil[i].gas_oil_contact_depth_ *= units.length;
            equil[i].gas_oil_cap_pressure_ *= units.pressure;
        }
    }
};

struct PVCDO : public SpecialBase
{
    std::vector<std::vector<double> > pvcdo_;

    virtual std::string name() const {return std::string("PVCDO");}

    virtual void read(std::istream& is)
    {
        while (!is.eof()) {
            std::vector<double> pvcdo;
            readVectorData(is, pvcdo);
            if (pvcdo.size() == 4) {
                pvcdo.push_back(0.0);
            }
            pvcdo_.push_back(pvcdo);

            int action = next_action(is); // 0:continue  1:return  2:throw
            if (action == 1) {
                return;     // Alphabetic char. Read next keyword.
            } else if (action == 2) {
                OPM_THROW(std::runtime_error, "Error reading PVCDO. Next character is "
                      <<  (char)is.peek());
            }
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int i=0; i<(int)pvcdo_.size(); ++i) {
            os << pvcdo_[i][0] << " " << pvcdo_[i][1] << " " << pvcdo_[i][2]
               << " " << pvcdo_[i][3] << " " << pvcdo_[i][4] << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        double volfac = units.liqvol_r/units.liqvol_s;
        for (int i=0; i<(int)pvcdo_.size(); ++i) {
            pvcdo_[i][0] *= units.pressure;
            pvcdo_[i][1] *= volfac;
            pvcdo_[i][2] *= units.compressibility;
            pvcdo_[i][3] *= units.viscosity;
            pvcdo_[i][4] *= units.compressibility;
        }
    }
};

struct TSTEP : public SpecialBase
{
    std::vector<double> tstep_;

    virtual std::string name() const {return std::string("TSTEP");}

    virtual void read(std::istream& is)
    {
        std::vector<double> tstep;
        readVectorData(is, tstep);
        if (!tstep.empty()) {
            tstep_.insert(tstep_.end(), tstep.begin(), tstep.end());
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        copy(tstep_.begin(), tstep_.end(),
             std::ostream_iterator<double>(os, " "));
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        int num_steps = tstep_.size();
        for (int i = 0; i < num_steps; ++i) {
            tstep_[i] *= units.time;
        }
    }
};

struct MultRec : public SpecialBase
{
    virtual void read(std::istream& is)
    {
#ifdef VERBOSE
        std::cout << "(dummy implementation)" << std::endl;
#endif
        const std::ctype<char>& ct = std::use_facet< std::ctype<char> >(std::locale::classic());
        is >> ignoreSlashLine;
        while (!is.eof()) {
            is >> ignoreWhitespace;
            std::streampos pos = is.tellg();
            char c;
            is.get(c);
            if (is.eof()) {
                return;
            }
            if (ct.is(std::ctype_base::alpha, c)) {
                std::string name;     // Unquoted name or new keyword?
                std::getline(is, name);
                if (name.rfind('/') != std::string::npos) {
                    continue;  // Unquoted name
                } else {
                    is.seekg(pos);
                    break;     // Read next keyword
                }
            } else if (ct.is(std::ctype_base::digit, c) || c== '.') {
                is >> ignoreSlashLine; // Decimal digit. Ignore data.
                continue;
            } else if (c== '\'') {
                is >> ignoreSlashLine; // Quote. Ignore data.
                continue;
            } else if(c == '-' && is.peek() == int('-')) {
                is >> ignoreLine;   // This line is a comment
                continue;
            } else if (c == '/' ) {
                 is >> ignoreLine;  // This line is a null record.
                 continue;          // (No data before slash)
            } else {
                is.putback(c);
                std::string temp;
                is >> temp;
                std::cout << "READ ERROR!  Next word is " << temp << std::endl;
            }
        }
    }

    virtual void convertToSI(const EclipseUnits&)
    {}

};


struct PLYVISC : public SpecialBase
{
    std::vector<double> concentration_;
    std::vector<double> factor_;

    virtual std::string name() const {return std::string("PLYVISC");}

    virtual void read(std::istream& is)
    {
        // Note. This function assumes that NTPVT = 1, and reads only one table.
        std::vector<double> plyvisc;
        readVectorData(is, plyvisc);
        for (int i=0; i<(int)plyvisc.size(); i+=2) {
            concentration_.push_back(plyvisc[i]);
            factor_.push_back(plyvisc[i+1]);
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int i=0; i<(int)concentration_.size(); ++i) {
            os << concentration_[i] << " " << factor_[i] << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int i=0; i<(int)concentration_.size(); ++i) {
            concentration_[i] *= units.polymer_density;
        }
    }
};


struct PLYROCK : public SpecialBase
{
    std::vector<double> plyrock_;

    virtual std::string name() const {return std::string("PLYROCK");}

    virtual void read(std::istream& is)
    {
        // Note. This function assumes that NTSFUN = 1, and reads only one line.
        plyrock_.resize(5,-1e00);
        plyrock_[3] = 1;  // Default value
        int num_to_read = 5;
        int num_read = readDefaultedVectorData(is, plyrock_, num_to_read);
        if (num_read == num_to_read) {
            ignoreSlashLine(is);
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int i=0; i<(int)plyrock_.size(); ++i) {
            os << plyrock_[i] << " ";
        }
        os << "\n\n";
    }

    virtual void convertToSI(const EclipseUnits& units)
    {

        plyrock_[2] *= units.polymer_density;
    }
};


struct PLYADS : public SpecialBase
{
    std::vector<double> local_concentration_;
    std::vector<double> adsorbed_concentration_;

    virtual std::string name() const {return std::string("PLYADS");}

    virtual void read(std::istream& is)
    {
        // Note. This function assumes that NTSFUN = 1, and reads only one table.
        std::vector<double> plyads;
        readVectorData(is, plyads);
        for (int i=0; i<(int)plyads.size(); i+=2) {
            local_concentration_.push_back(plyads[i]);
            adsorbed_concentration_.push_back(plyads[i+1]);
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int i=0; i<(int)local_concentration_.size(); ++i) {
            os << local_concentration_[i] << " " << adsorbed_concentration_[i] << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int i=0; i<(int)local_concentration_.size(); ++i) {
            local_concentration_[i] *= units.polymer_density;
        }
    }
};


struct PLYMAX : public SpecialBase
{
    std::vector<double> plymax_;

    virtual std::string name() const {return std::string("PLYMAX");}

    virtual void read(std::istream& is)
    {
        // Note. This function assumes that NTMISC = 1, and reads only one line.
        plymax_.resize(2);
        readVectorData(is, plymax_);
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int i=0; i<(int)plymax_.size(); ++i) {
            os << plymax_[i] << " ";
        }
        os << "\n\n";
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        plymax_[0] *= units.polymer_density;
        plymax_[1] *= units.polymer_density;
    }
};


struct TLMIXPAR : public SpecialBase
{
    std::vector<double> tlmixpar_;

    virtual std::string name() const {return std::string("TLMIXPAR");}

    virtual void read(std::istream& is)
    {
        // Note. This function assumes that NTMISC = 1, and reads only one record.
        tlmixpar_.resize(2, -1e100);
        int num_to_read = 2;
        int num_read = readDefaultedVectorData(is, tlmixpar_, num_to_read);
        if (tlmixpar_[1] < 0) {
            tlmixpar_[1] = tlmixpar_[0];
        }
        if (num_read == num_to_read) {
            ignoreSlashLine(is);
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int i=0; i<(int)tlmixpar_.size(); ++i) {
            os << tlmixpar_[i] << " ";
        }
        os << "\n\n";
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        static_cast<void>(units); // Suppress "unused parameter" warning.
    }
};

/// Class holding a data line of keyword WPOLYMER
struct WpolymerLine
{
    std::string well_;  // Well name, well name template or well list template
    double polymer_concentration_;
    double salt_concentration_;
    std::string polymer_group_;
    std::string salt_group_;

    WpolymerLine()
    {
        well_ = polymer_group_ = salt_group_ = "";
        polymer_concentration_ = salt_concentration_ = 0.0;
    }
};

struct WPOLYMER : public SpecialBase
{
    std::vector<WpolymerLine> wpolymer_;

    virtual std::string name() const {return std::string("WPOLYMER");}

    virtual void read(std::istream& is)
    {
        while(is) {
            std::string wellname = readString(is);
            if (wellname[0] == '/') {
                is >> ignoreLine;
                break;
            }
            while (wellname.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                wellname = readString(is);
            }
            WpolymerLine wpolymer_line;
            wpolymer_line.well_ = wellname;
            is >> wpolymer_line.polymer_concentration_;
            is >> wpolymer_line.salt_concentration_;
            std::string group = readString(is);
            if (group[0] == '/') {
                is >> ignoreLine;
            } else {
                wpolymer_line.polymer_group_ = group;
                group = readString(is);
                if (group[0] == '/') {
                    is >> ignoreLine;
                } else {
                    wpolymer_line.salt_group_ = group;
                    is >> ignoreLine;
                }
            }
            wpolymer_.push_back(wpolymer_line);
        }
    }

    virtual void write(std::ostream& os) const
    {
        os << name() << '\n';
        for (int i=0; i<(int)wpolymer_.size(); ++i) {
            os << wpolymer_[i].well_
               <<  "  " << wpolymer_[i].polymer_concentration_
               <<  "  " << wpolymer_[i].salt_concentration_
               <<  "  " << wpolymer_[i].polymer_group_
               <<  "  " << wpolymer_[i].salt_group_;
            os << '\n';
        }
        os << '\n';
    }

    virtual void convertToSI(const EclipseUnits& units)
    {
        for (int i=0; i<(int)wpolymer_.size(); ++i) {
            wpolymer_[i].polymer_concentration_ *= units.polymer_density;
            wpolymer_[i].salt_concentration_ *= units.polymer_density;
        }
    }
};

struct ENDSCALE : public SpecialBase {
    std::string dir_switch_;
    std::string revers_switch_;
    int n_tab_;
    int n_node_;

    virtual std::string name() const {
        return std::string("ENDSCALE");
    }

    virtual void read(std::istream & is) {
        dir_switch_ = std::string("NODIR");
        revers_switch_ = std::string("REVERS");
        n_tab_ = 1;
        n_node_ = 20;
        while (!is.eof()) {
            std::string tmp = readString(is);
            while (tmp.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                tmp = readString(is);
            }
            if (tmp[0] == '/') {
                is >> ignoreLine;
                break;
            }
            dir_switch_ = tmp;
            tmp = readString(is);
            if (tmp[0] == '/') {
                is >> ignoreLine;
                break;
            }
            revers_switch_ = tmp;
            is >> ignoreWhitespace;
            if(is.peek() == int('/')) {
                is >> ignoreLine;
                return;
            }
            is >> n_tab_;
            is >> ignoreWhitespace;
            if(is.peek() == int('/')) {
                is >> ignoreLine;
                return;
            }
            is >> n_node_;
            is >> ignoreLine;
            break;
        }
    }

    virtual void write(std::ostream & os) {
        os << name() << '\n';
        os << dir_switch_ << "   " << revers_switch_ << "   "
           << n_tab_ << "   " << n_node_ << '\n';
    }

    virtual void convertToSI(const EclipseUnits&) {
    }
};

struct SCALECRS : public SpecialBase {
    std::string scalecrs_;

    virtual std::string name() const {
        return std::string("SCALECRS");
    }

    virtual void read(std::istream & is) {
        scalecrs_ = std::string("NO");
        while (!is.eof()) {
            std::string tmp = readString(is);
            while (tmp.find("--") == 0) {
                // This line is a comment
                is >> ignoreLine;
                tmp = readString(is);
            }
            if (tmp[0] == '/') {
                is >> ignoreLine;
                break;
            } else if (tmp[0] == 'Y') {
                scalecrs_ = std::string("YES");
            }
            is >> ignoreLine;
            break;
        }
    }

    virtual void write(std::ostream & os) {
        os << name() << '\n';
        os << scalecrs_ << '\n';
    }

    virtual void convertToSI(const EclipseUnits&) {
    }
};

struct ENPTVD : public SpecialBase {
    std::vector<std::vector<std::vector<double> > > table_;
    std::vector<bool> mask_;

    virtual std::string name() const {
        return std::string("ENPTVD");
    }

    virtual void read(std::istream & is) {
        table_.resize(5);
        std::vector<std::vector<double> > sub_table(5);
        while (!is.eof()) {
            if(is.peek() == int('/')) {
                if (sub_table[0].empty() && !(table_[0].empty())) {
                    is >> ignoreLine;
                    break;
                } else {
                    OPM_THROW(std::runtime_error, "Error reading ENPTVD data - none or incomplete table.");
                }
            }
            std::vector<double> data(9,-1.0);
            int nread = readDefaultedVectorData(is, data, 9);
            if (nread != 9) {
                OPM_THROW(std::runtime_error, "Error reading ENPTVD data - depth and 8 saturations pr line.");
            }
            if (data[0] == -1.0) {
                OPM_THROW(std::runtime_error, "Error reading ENPTVD data - depth can not be defaulted.");
            }
            if ((data[4] != -1.0) || (data[5] != -1.0) || (data[6] != -1.0) || (data[8] != -1.0)) {
                OPM_THROW(std::runtime_error, "Error reading ENPTVD data - non-default values in column 5-7,9 not supported.");
            }
            sub_table[0].push_back(data[0]); //depth
            sub_table[1].push_back(data[1]); //swl
            sub_table[2].push_back(data[2]); //swcr
            sub_table[3].push_back(data[3]); //swu
            sub_table[4].push_back(data[7]); //sowcr
            is >> ignoreWhitespace;
            if(is.peek() == int('/')) {
                is >> ignoreLine;
                if (sub_table[0].size() >= 2) {
                    insertDefaultValues(sub_table, 5, -1.0, false);
                    std::vector<std::vector<double> >::iterator it_sub = sub_table.begin();
                    for(std::vector<std::vector<std::vector<double> > >::size_type i=0; i<table_.size(); ++i) {
                        table_[i].push_back(*it_sub);
                        (*it_sub).clear();
                        ++it_sub;
                    }
                } else {
                    OPM_THROW(std::runtime_error, "Error reading ENPTVD data - minimum 2 lines pr sub-table.");
                }
            }
        }
        for (std::vector<std::vector<std::vector<double> > >::size_type i=1; i<table_.size(); ++i) {
            mask_.push_back(false);
            for (std::vector<std::vector<double> >::size_type j=0; j<table_[0].size(); ++j) {
                if (table_[i][j][0] != -1.0) {
                    mask_.back() = true;
                    break;
                }
            }
        }
    }

    virtual void write(std::ostream & os) const {
        os << name() << '\n';
        std::cout << "-----depth-------swl------swcr-------swu-----sowcr" << std::endl;
        for (std::vector<std::vector<double> >::size_type j=0; j<table_[0].size(); ++j) {
            std::cout << "--------------------------------------------------" << std::endl;
            for (std::vector<double>::size_type k=0; k<table_[0][j].size(); ++k) {
                for (std::vector<std::vector<std::vector<double> > >::size_type i=0; i<table_.size(); ++i) {
                    std::cout << std::setw(10) << table_[i][j][k];
                }
                std::cout << std::endl;
            }
        }
    }

    virtual void convertToSI(const EclipseUnits& units) {
        //depths
        for (std::vector<std::vector<double> >::size_type j=0; j<table_[0].size(); ++j) {
            for (std::vector<double>::size_type k=0; k<table_[0][j].size(); ++k) {
                table_[0][j][k] *= units.length;
            }
        }
    }
};

struct ENKRVD : public SpecialBase {
    std::vector<std::vector<std::vector<double> > > table_;
    std::vector<bool> mask_;

    virtual std::string name() const {
        return std::string("ENKRVD");
    }

    virtual void read(std::istream & is) {
        table_.resize(5);
        std::vector<std::vector<double> > sub_table(5);
        while (!is.eof()) {
            if(is.peek() == int('/')) {
                if (sub_table[0].empty() && !(table_[0].empty())) {
                    is >> ignoreLine;
                    break;
                } else {
                    OPM_THROW(std::runtime_error, "Error reading ENKRVD data - none or incomplete table.");
                }
            }
            std::vector<double> data(8,-1.0);
            int nread = readDefaultedVectorData(is, data, 8);
            if (nread != 8) {
                OPM_THROW(std::runtime_error, "Error reading ENKRVD data - depth and 7 relperms pr line.");
            }
            if (data[0] == -1.0) {
                OPM_THROW(std::runtime_error, "Error reading ENKRVD data - depth can not be defaulted.");
            }
            if ((data[2] != -1.0) || (data[5] != -1.0) || (data[6] != -1.0)) {
               OPM_THROW(std::runtime_error, "Error reading ENKRVD data - non-default values in column 3,6-7 not supported.");
            }
            sub_table[0].push_back(data[0]); //depth
            sub_table[1].push_back(data[1]); //krw
            sub_table[2].push_back(data[3]); //kro
            sub_table[3].push_back(data[4]); //krw(sowcr)
            sub_table[4].push_back(data[7]); //kro(swcr)
            is >> ignoreWhitespace;
            if(is.peek() == int('/')) {
                is >> ignoreLine;
                if (sub_table[0].size() >= 2) {
                    insertDefaultValues(sub_table, 5, -1.0, false);
                    std::vector<std::vector<double> >::iterator it_sub = sub_table.begin();
                    for(std::vector<std::vector<std::vector<double> > >::size_type i=0; i<table_.size(); ++i) {
                        table_[i].push_back(*it_sub);
                        (*it_sub).clear();
                        ++it_sub;
                    }
                } else {
                    OPM_THROW(std::runtime_error, "Error reading ENKRVD data - minimum 2 lines pr sub-table.");
                }
            }
        }
        for (std::vector<std::vector<std::vector<double> > >::size_type i=1; i<table_.size(); ++i) {
            mask_.push_back(false);
            for (std::vector<std::vector<double> >::size_type j=0; j<table_[0].size(); ++j) {
                if (table_[i][j][0] != -1.0) {
                    mask_.back() = true;
                    break;
                }
            }
        }
    }


    virtual void write(std::ostream & os) const {
        os << name() << '\n';
        std::cout << "-----depth-------krw------krow------krwr-----krorw" << std::endl;
        for (std::vector<std::vector<double> >::size_type j=0; j<table_[0].size(); ++j) {
            std::cout << "--------------------------------------------------" << std::endl;
            for (std::vector<double>::size_type k=0; k<table_[0][j].size(); ++k) {
                for (std::vector<std::vector<std::vector<double> > >::size_type i=0; i<table_.size(); ++i) {
                    std::cout << std::setw(10) << table_[i][j][k];
                }
                std::cout << std::endl;
            }
        }
    }

    virtual void convertToSI(const EclipseUnits& units) {
        //depths
        for (std::vector<std::vector<double> >::size_type j=0; j<table_[0].size(); ++j) {
            for (std::vector<double>::size_type k=0; k<table_[0][j].size(); ++k) {
                table_[0][j][k] *= units.length;
            }
        }
    }
};


// The following fields only have a dummy implementation
// that allows us to ignore them.
struct SWFN : public MultRec {};
struct SOF2 : public MultRec {};
struct TUNING : public MultRec {};


} // End of namespace Opm

#endif // OPM_SPECIALECLIPSEFIELDS_HEADER

