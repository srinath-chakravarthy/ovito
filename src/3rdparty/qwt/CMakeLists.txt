###############################################################################
# 
#  Copyright (2016) Alexander Stukowski
#
#  This file is part of OVITO (Open Visualization Tool).
#
#  OVITO is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  OVITO is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

# Build the library.
SET(QWT_SOURCE_FILES
    qwt_abstract_scale_draw.cpp
    qwt_clipper.cpp
    qwt_color_map.cpp
    qwt_column_symbol.cpp
    qwt_date.cpp
    qwt_date_scale_draw.cpp
    qwt_date_scale_engine.cpp
    qwt_dyngrid_layout.cpp
    qwt_event_pattern.cpp
    qwt_graphic.cpp
    qwt_interval.cpp
    qwt_interval_symbol.cpp
    qwt_math.cpp
    qwt_magnifier.cpp
    qwt_null_paintdevice.cpp
    qwt_painter.cpp
    qwt_painter_command.cpp
    qwt_panner.cpp
    qwt_picker.cpp
    qwt_picker_machine.cpp
    qwt_pixel_matrix.cpp
    qwt_point_3d.cpp
    qwt_point_polar.cpp
    qwt_round_scale_draw.cpp
    qwt_scale_div.cpp
    qwt_scale_draw.cpp
    qwt_scale_map.cpp
    qwt_spline.cpp
    qwt_scale_engine.cpp
    qwt_symbol.cpp
    qwt_system_clock.cpp
    qwt_text_engine.cpp
    qwt_text_label.cpp
    qwt_text.cpp
    qwt_transform.cpp
    qwt_widget_overlay.cpp

	# QwtPlot files:
	qwt_curve_fitter.cpp
	qwt_abstract_legend.cpp
	qwt_legend.cpp
	qwt_legend_data.cpp
	qwt_legend_label.cpp
	qwt_plot.cpp
	qwt_plot_renderer.cpp
	qwt_plot_xml.cpp
	qwt_plot_axis.cpp
	qwt_plot_curve.cpp
	qwt_plot_dict.cpp
	qwt_plot_directpainter.cpp
	qwt_plot_grid.cpp
	qwt_plot_histogram.cpp
	qwt_plot_item.cpp
	qwt_plot_abstract_barchart.cpp
	qwt_plot_barchart.cpp
	qwt_plot_multi_barchart.cpp
	qwt_plot_intervalcurve.cpp
	qwt_plot_zoneitem.cpp
	qwt_plot_tradingcurve.cpp
	qwt_plot_spectrogram.cpp
	qwt_plot_spectrocurve.cpp
	qwt_plot_scaleitem.cpp
	qwt_plot_legenditem.cpp
	qwt_plot_seriesitem.cpp
	qwt_plot_shapeitem.cpp
	qwt_plot_marker.cpp
	qwt_plot_textlabel.cpp
	qwt_plot_layout.cpp
	qwt_plot_canvas.cpp
	qwt_plot_panner.cpp
	qwt_plot_rasteritem.cpp
	qwt_plot_picker.cpp
	qwt_plot_zoomer.cpp
	qwt_plot_magnifier.cpp
	qwt_plot_rescaler.cpp
	qwt_point_mapper.cpp
	qwt_raster_data.cpp
	qwt_matrix_raster_data.cpp
	qwt_sampling_thread.cpp
	qwt_series_data.cpp
	qwt_point_data.cpp
	qwt_scale_widget.cpp
)

# Build the library.
ADD_LIBRARY(Qwt SHARED ${QWT_SOURCE_FILES}) 

# Link to Qt5.
TARGET_LINK_LIBRARIES(Qwt PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport Qt5::Concurrent)

# Some compile time flags for Qwt.
TARGET_COMPILE_DEFINITIONS(Qwt PRIVATE QWT_NO_SVG QWT_NO_OPENGL)

SET_TARGET_PROPERTIES(Qwt PROPERTIES MACOSX_RPATH TRUE)

# Export this target.
INSTALL(TARGETS Qwt EXPORT OVITO 
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")

