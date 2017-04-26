///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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


#include <plugins/particles/Particles.h>
#include <plugins/particles/data/SimulationCell.h>
#include <core/scene/objects/DataObject.h>

namespace Ovito { namespace Particles {

/**
 * \brief Stores the geometry and boundary conditions of a simulation box.
 *
 * The simulation box geometry is a parallelepiped defined by three edge vectors.
 * A fourth vector specifies the origin of the simulation box in space.
 */
class OVITO_PARTICLES_EXPORT SimulationCellObject : public DataObject
{
public:

	/// \brief Constructor. Creates an empty simulation cell.
	Q_INVOKABLE SimulationCellObject(DataSet* dataset) : DataObject(dataset),
		_cellVector1(Vector3::Zero()), _cellVector2(Vector3::Zero()), _cellVector3(Vector3::Zero()),
		_cellOrigin(Point3::Origin()), _pbcX(false), _pbcY(false), _pbcZ(false), _is2D(false) {
		init(dataset);
	}

	/// \brief Constructs a cell from the given cell data structure.
	explicit SimulationCellObject(DataSet* dataset, const SimulationCell& data) : DataObject(dataset),
			_cellVector1(data.matrix().column(0)), _cellVector2(data.matrix().column(1)), _cellVector3(data.matrix().column(2)),
			_cellOrigin(Point3::Origin() + data.matrix().column(3)), _pbcX(data.pbcFlags()[0]), _pbcY(data.pbcFlags()[1]), _pbcZ(data.pbcFlags()[2]), _is2D(data.is2D()) {
		init(dataset);
	}

	/// \brief Constructs a cell from three vectors specifying the cell's edges.
	/// \param a1 The first edge vector.
	/// \param a2 The second edge vector.
	/// \param a3 The third edge vector.
	/// \param origin The origin position.
	SimulationCellObject(DataSet* dataset, const Vector3& a1, const Vector3& a2, const Vector3& a3,
			const Point3& origin = Point3::Origin(), bool pbcX = false, bool pbcY = false, bool pbcZ = false, bool is2D = false) :
		DataObject(dataset),
		_cellVector1(a1), _cellVector2(a2), _cellVector3(a3),
		_cellOrigin(origin), _pbcX(pbcX), _pbcY(pbcY), _pbcZ(pbcZ), _is2D(is2D) {
		init(dataset);
	}

	/// \brief Constructs a cell from a matrix that specifies its shape and position in space.
	/// \param cellMatrix The matrix
	SimulationCellObject(DataSet* dataset, const AffineTransformation& cellMatrix, bool pbcX = false, bool pbcY = false, bool pbcZ = false, bool is2D = false) :
		DataObject(dataset),
		_cellVector1(cellMatrix.column(0)), _cellVector2(cellMatrix.column(1)), _cellVector3(cellMatrix.column(2)),
		_cellOrigin(Point3::Origin() + cellMatrix.column(3)), _pbcX(pbcX), _pbcY(pbcY), _pbcZ(pbcZ), _is2D(is2D) {
		init(dataset);
	}

	/// \brief Constructs a cell with an axis-aligned box shape.
	/// \param box The axis-aligned box.
	/// \param pbcX Specifies whether periodic boundary conditions are enabled in the X direction.
	/// \param pbcY Specifies whether periodic boundary conditions are enabled in the Y direction.
	/// \param pbcZ Specifies whether periodic boundary conditions are enabled in the Z direction.
	SimulationCellObject(DataSet* dataset, const Box3& box, bool pbcX = false, bool pbcY = false, bool pbcZ = false, bool is2D = false) :
		DataObject(dataset),
		_cellVector1(box.sizeX(), 0, 0), _cellVector2(0, box.sizeY(), 0), _cellVector3(0, 0, box.sizeZ()),
		_cellOrigin(box.minc), _pbcX(pbcX), _pbcY(pbcY), _pbcZ(pbcZ), _is2D(is2D) {
		init(dataset);
		OVITO_ASSERT_MSG(box.sizeX() >= 0 && box.sizeY() >= 0 && box.sizeZ() >= 0, "SimulationCellObject constructor", "The simulation box must have a non-negative volume.");
	}

	/// \brief Sets the cell geometry to match the given cell data structure.
	void setData(const SimulationCell& data, bool setBoundaryFlags = true) {
		setCellVector1(data.matrix().column(0));
		setCellVector2(data.matrix().column(1));
		setCellVector3(data.matrix().column(2));
		setCellOrigin(Point3::Origin() + data.matrix().column(3));
		if(setBoundaryFlags) {
			setPbcX(data.pbcFlags()[0]);
			setPbcY(data.pbcFlags()[1]);
			setPbcZ(data.pbcFlags()[2]);
			setIs2D(data.is2D());
		}
	}

	/// \brief Returns a simulation cell data structure that stores the cell's properties.
	SimulationCell data() const {
		SimulationCell data;
		data.setMatrix(cellMatrix());
		data.setPbcFlags(pbcX(), pbcY(), pbcZ());
		data.set2D(is2D());
		return data;
	}

	/// \brief Returns the geometry of the simulation cell as a 3x4 matrix.
	/// \return The shape matrix of the simulation cell.
	///         The first three matrix columns specify the three edge vectors;
	///         the fourth matrix column specifies the translation of the cell origin.
	AffineTransformation cellMatrix() const {
		return {
			cellVector1(),
			cellVector2(),
			cellVector3(),
			cellOrigin() - Point3::Origin()
		};
	}

	/// \brief Changes the cell's shape.
	/// \param shape Specifies the new shape matrix for the simulation cell.
	///              The first three matrix columns contain the three edge vectors.
	///              The fourth matrix column specifies the translation of the cell's origin.
	///
	/// \undoable
	void setCellMatrix(const AffineTransformation& shape) {
		setCellVector1(shape.column(0));
		setCellVector2(shape.column(1));
		setCellVector3(shape.column(2));
		setCellOrigin(Point3::Origin() + shape.column(3));
	}

	/// Returns inverse of the simulation cell matrix.
	/// This matrix maps the simulation cell to the unit cube ([0,1]^3).
	AffineTransformation reciprocalCellMatrix() const {
		return cellMatrix().inverse();
	}

	/// Computes the (positive) volume of the three-dimensional cell.
	FloatType volume3D() const {
		return std::abs(cellVector1().dot(cellVector2().cross(cellVector3())));
	}

	/// Computes the (positive) volume of the two-dimensional cell.
	FloatType volume2D() const {
		return cellVector1().cross(cellVector2()).length();
	}

	/// \brief Enables or disables periodic boundary conditions in the three spatial directions.
	void setPBCFlags(const std::array<bool,3>& flags) {
		setPbcX(flags[0]);
		setPbcY(flags[1]);
		setPbcZ(flags[2]);
	}

	/// \brief Returns the periodic boundary flags in all three spatial directions.
	std::array<bool,3> pbcFlags() const {
		return {{ pbcX(), pbcY(), pbcZ() }};
	}

	/// \brief Returns the title of this object.
	virtual QString objectTitle() override { return tr("Simulation cell"); }

protected:

	/// Creates the storage for the internal parameters.
	void init(DataSet* dataset);

	/// Stores the first cell edge.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Vector3, cellVector1, setCellVector1);
	/// Stores the second cell edge.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Vector3, cellVector2, setCellVector2);
	/// Stores the third cell edge.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Vector3, cellVector3, setCellVector3);
	/// Stores the cell origin.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Point3, cellOrigin, setCellOrigin);

	/// Specifies periodic boundary condition in the X direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, pbcX, setPbcX);
	/// Specifies periodic boundary condition in the Y direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, pbcY, setPbcY);
	/// Specifies periodic boundary condition in the Z direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, pbcZ, setPbcZ);

	/// Stores the dimensionality of the system.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, is2D, setIs2D);

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("ClassNameAlias", "SimulationCell");	// This for backward compatibility with files written by Ovito 2.4 and older.
};

}	// End of namespace
}	// End of namespace


