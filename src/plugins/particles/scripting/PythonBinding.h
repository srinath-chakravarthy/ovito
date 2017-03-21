///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once


#include <plugins/pyscript/PyScript.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/objects/BondPropertyObject.h>
#include <plugins/particles/objects/FieldQuantityObject.h>
#include <plugins/particles/import/InputColumnMapping.h>
#include <plugins/particles/export/OutputColumnMapping.h>

namespace pybind11 { namespace detail {

	/// Automatic Python string <--> ParticlePropertyReference conversion
    template<> struct type_caster<Ovito::Particles::ParticlePropertyReference> {
    public:
        PYBIND11_TYPE_CASTER(Ovito::Particles::ParticlePropertyReference, _("ParticlePropertyReference"));

        bool load(handle src, bool) {
			using namespace Ovito;
			using namespace Ovito::Particles;

			if(!src) return false;
			if(src.is_none())
				return true;

			try {
				ParticleProperty::Type ptype = src.cast<ParticleProperty::Type>();
				if(ptype == ParticleProperty::UserProperty)
					throw Exception("User-defined particle property without a name is not acceptable.");
				value = ParticlePropertyReference(ptype);
				return true;
			}
			catch(const cast_error&) {}
			
			QString str;
			try {
				str = src.cast<QString>();
			}
			catch(const cast_error&) {
				return false;
			}

			QStringList parts = str.split(QChar('.'));
			if(parts.length() > 2)
				throw Exception("Too many dots in particle property name string.");
			else if(parts.length() == 0 || parts[0].isEmpty())
				throw Exception("Particle property name string is empty.");

			// Determine property type.
			QString name = parts[0];
			ParticleProperty::Type type = ParticleProperty::standardPropertyList().value(name, ParticleProperty::UserProperty);

			// Determine vector component.
			int component = -1;
			if(parts.length() == 2) {
				// First try to convert component to integer.
				bool ok;
				component = parts[1].toInt(&ok);
				if(!ok) {
					if(type != ParticleProperty::UserProperty) {
						// Perhaps the standard property's component name was used instead of an integer.
						const QString componentName = parts[1].toUpper();
						QStringList standardNames = ParticleProperty::standardPropertyComponentNames(type);
						component = standardNames.indexOf(componentName);
						if(component < 0)
							throw Exception(QString("Component name '%1' is not defined for particle property '%2'. Possible components are: %3").arg(parts[1]).arg(parts[0]).arg(standardNames.join(',')));
					}
					else {
						// Assume user-defined properties cannot be vectors.
						component = -1;
						name = parts.join(QChar('.'));
					}
				}
			}
			if(type == ParticleProperty::UserProperty)
				value = ParticlePropertyReference(name, component);
			else
				value = ParticlePropertyReference(type, component);
			return true;
		}

        static handle cast(const Ovito::Particles::ParticlePropertyReference& src, return_value_policy /* policy */, handle /* parent */) {			
        	object s = pybind11::cast(src.nameWithComponent());
			return s.release();
        }
    };

	/// Automatic Python string <--> BondPropertyReference conversion
    template<> struct type_caster<Ovito::Particles::BondPropertyReference> {
    public:
        PYBIND11_TYPE_CASTER(Ovito::Particles::BondPropertyReference, _("BondPropertyReference"));

        bool load(handle src, bool) {
			using namespace Ovito;
			using namespace Ovito::Particles;
			try {
				BondProperty::Type ptype = src.cast<BondProperty::Type>();
				if(ptype == BondProperty::UserProperty)
					throw Exception("User-defined bond property without a name is not acceptable.");
				value = BondPropertyReference(ptype);
				return true;
			}
			catch(const cast_error&) {}
			
			QString str;
			try {
				str = src.cast<QString>();
			}
			catch(const cast_error&) {
				return false;
			}

			QStringList parts = str.split(QChar('.'));
			if(parts.length() > 2)
				throw Exception("Too many dots in bond property name string.");
			else if(parts.length() == 0 || parts[0].isEmpty())
				throw Exception("Bond property name string is empty.");

			// Determine property type.
			QString name = parts[0];
			BondProperty::Type type = BondProperty::standardPropertyList().value(name, BondProperty::UserProperty);

			// Determine vector component.
			int component = -1;
			if(parts.length() == 2) {
				// First try to convert component to integer.
				bool ok;
				component = parts[1].toInt(&ok);
				if(!ok) {
					if(type != BondProperty::UserProperty) {
						// Perhaps the standard property's component name was used instead of an integer.
						const QString componentName = parts[1].toUpper();
						QStringList standardNames = BondProperty::standardPropertyComponentNames(type);
						component = standardNames.indexOf(componentName);
						if(component < 0)
							throw Exception(QString("Component name '%1' is not defined for bond property '%2'. Possible components are: %3").arg(parts[1]).arg(parts[0]).arg(standardNames.join(',')));
					}
					else {
						// Assume user-defined properties cannot be vectors.
						component = -1;
						name = parts.join(QChar('.'));
					}
				}
			}
			if(type == BondProperty::UserProperty)
				value = BondPropertyReference(name, component);
			else
				value = BondPropertyReference(type, component);
			return true;
		}

        static handle cast(const Ovito::Particles::BondPropertyReference& src, return_value_policy /* policy */, handle /* parent */) {			
        	object s = pybind11::cast(src.nameWithComponent());
			return s.release();
        }
    };

	/// Automatic Python string <--> FieldQuantityReference conversion
    template<> struct type_caster<Ovito::Particles::FieldQuantityReference> {
    public:
        PYBIND11_TYPE_CASTER(Ovito::Particles::FieldQuantityReference, _("FieldQuantityReference"));

        bool load(handle src, bool) {
			using namespace Ovito;
			using namespace Ovito::Particles;
			
			QString str;
			try {
				str = src.cast<QString>();
			}
			catch(const cast_error&) {
				return false;
			}

			QStringList parts = str.split(QChar('.'));
			if(parts.length() > 2)
				throw Exception("Too many dots in field quantity name string.");
			else if(parts.length() == 0 || parts[0].isEmpty())
				throw Exception("Field quantity name string is empty.");
			QString name = parts[0];

			// Determine vector component.
			int component = -1;
			if(parts.length() == 2) {
				// Try to convert component to integer.
				bool ok;
				component = parts[1].toInt(&ok);
				if(!ok) {
					// Assume its not a vector quantity.
					component = -1;
					name = parts.join(QChar('.'));
				}
			}
			value = FieldQuantityReference(name, component);
			return true;
		}

        static handle cast(const Ovito::Particles::FieldQuantityReference& src, return_value_policy /* policy */, handle /* parent */) {			
        	object s = pybind11::cast(src.nameWithComponent());
			return s.release();
        }
    };

	/// Automatic Python string list <--> InputColumnMapping conversion
    template<> struct type_caster<Ovito::Particles::InputColumnMapping> {
    public:
        PYBIND11_TYPE_CASTER(Ovito::Particles::InputColumnMapping, _("InputColumnMapping"));

        bool load(handle src, bool) {
			try {
				if(!isinstance<sequence>(src)) return false;
				sequence seq = reinterpret_borrow<sequence>(src);
				value.resize(seq.size());
				for(size_t i = 0; i < value.size(); i++) {
					Ovito::Particles::ParticlePropertyReference pref = seq[i].cast<Ovito::Particles::ParticlePropertyReference>();
					if(!pref.isNull()) {
						if(pref.type() != Ovito::Particles::ParticleProperty::UserProperty)
							value[i].mapStandardColumn(pref.type(), pref.vectorComponent());
						else
							value[i].mapCustomColumn(pref.name(), qMetaTypeId<Ovito::FloatType>(), pref.vectorComponent());
					}
				}
				return true;
			}
			catch(const cast_error&) {}
			return false;
        }

        static handle cast(const Ovito::Particles::InputColumnMapping& src, return_value_policy /* policy */, handle /* parent */) {
        	list ls;
			for(const auto& col : src)
				ls.append(pybind11::cast(col.property.nameWithComponent()));
			return ls.release();
        }
    };	

	/// Automatic Python string list <--> OutputColumnMapping conversion
    template<> struct type_caster<Ovito::Particles::OutputColumnMapping> {
    public:
        PYBIND11_TYPE_CASTER(Ovito::Particles::OutputColumnMapping, _("OutputColumnMapping"));

        bool load(handle src, bool) {
			try {
				if(!isinstance<sequence>(src)) return false;
				sequence seq = reinterpret_borrow<sequence>(src);
				value.reserve(seq.size());
				for(size_t i = 0; i < seq.size(); i++) {
					value.push_back(seq[i].cast<Ovito::Particles::ParticlePropertyReference>());
				}
				return true;
			}
			catch(const cast_error&) {}
			return false;
        }

        static handle cast(const Ovito::Particles::OutputColumnMapping& src, return_value_policy /* policy */, handle /* parent */) {
        	list ls;
			for(const auto& col : src)
				ls.append(pybind11::cast(col));
			return ls.release();
        }
    };	

}} // namespace pybind11::detail


