/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/

#pragma once

namespace OrthancStone {

  enum MessageType
  {
    MessageType_Widget_GeometryChanged,
    MessageType_Widget_ContentChanged,

    MessageType_LayerSource_GeometryReady,   // instance tags have been loaded
    MessageType_LayerSource_GeometryError,
    MessageType_LayerSource_ContentChanged,
    MessageType_LayerSource_SliceChanged,
    MessageType_LayerSource_ImageReady,      // instance pixels data have been loaded
    MessageType_LayerSource_LayerReady,      // layer is ready to be rendered
    MessageType_LayerSource_LayerError,

    MessageType_SliceLoader_GeometryReady,
    MessageType_SliceLoader_GeometryError,
    MessageType_SliceLoader_ImageReady,
    MessageType_SliceLoader_ImageError,

    MessageType_HttpRequestSuccess,
    MessageType_HttpRequestError,

    MessageType_OrthancApi_InternalGetJsonResponseReady,
    MessageType_OrthancApi_InternalGetJsonResponseError,

    MessageType_OrthancApi_GenericGetJson_Ready,
    MessageType_OrthancApi_GenericGetBinary_Ready,
    MessageType_OrthancApi_GenericHttpError_Ready,
    MessageType_OrthancApi_GenericEmptyResponse_Ready,

    // used in unit tests only
    MessageType_Test1,
    MessageType_Test2,

    MessageType_CustomMessage // Custom messages ids ust be greater than this (this one must remain in last position)
  };
}
