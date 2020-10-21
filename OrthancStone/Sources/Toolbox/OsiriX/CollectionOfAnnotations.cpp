/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#include "CollectionOfAnnotations.h"

#include "ArrayValue.h"
#include "IntegerValue.h"

#include <OrthancException.h>

#include <cassert>

namespace OrthancStone
{
  namespace OsiriX
  {
    static void GetAttributes(std::map<std::string, std::string>& target,
                              const pugi::xml_node& node)
    {
      for (pugi::xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute())
      {
        target[attr.name()] = attr.value();
      }
    }

      
    CollectionOfAnnotations::~CollectionOfAnnotations()
    {
      for (size_t i = 0; i < annotations_.size(); i++)
      {
        assert(annotations_[i] != NULL);
        delete annotations_[i];
      }
    }


    const Annotation& CollectionOfAnnotations::GetAnnotation(size_t i) const
    {
      if (i >= annotations_.size())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        assert(annotations_[i] != NULL);
        return *annotations_[i];
      }
    }
    

    void CollectionOfAnnotations::AddAnnotation(Annotation* annotation)
    {
      if (annotation == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
      else
      {
        annotations_.push_back(annotation);
      }
    }

    void CollectionOfAnnotations::ParseXml(const std::string& xml)
    {
      pugi::xml_document doc;
      pugi::xml_parse_result result = doc.load_buffer(xml.empty() ? NULL : xml.c_str(), xml.size());
      if (!result)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
        
      const pugi::xml_node& root = doc.document_element();
      if (std::string(root.name()) != "plist" ||
          !root.first_child() ||
          root.first_child() != root.last_child())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }

      std::map<std::string, std::string> attributes;
      GetAttributes(attributes, root);

      std::map<std::string, std::string>::const_iterator version = attributes.find("version");
      if (version == attributes.end() ||
          version->second != "1.0")
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }

      std::unique_ptr<IValue> value(IValue::Parse(root.first_child()));

      const DictionaryValue& dict = dynamic_cast<const DictionaryValue&>(*value);

      std::set<std::string> annotations;
      dict.GetMembers(annotations);

      for (std::set<std::string>::const_iterator
             it = annotations.begin(); it != annotations.end(); ++it)
      {
        const ArrayValue& images = dynamic_cast<const ArrayValue&>(dict.GetValue(*it));

        for (size_t i = 0; i < images.GetSize(); i++)
        {
          const DictionaryValue& image = dynamic_cast<const DictionaryValue&>(images.GetValue(i));
          const IntegerValue& number = dynamic_cast<const IntegerValue&>(image.GetValue("NumberOfROIs"));
          const ArrayValue& rois = dynamic_cast<const ArrayValue&>(image.GetValue("ROIs"));
          
          if (static_cast<int64_t>(rois.GetSize()) != number.GetValue())
          {
            throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
          }

          for (size_t j = 0; j < rois.GetSize(); j++)
          {
            const DictionaryValue& roi = dynamic_cast<const DictionaryValue&>(rois.GetValue(i));

            std::unique_ptr<Annotation> annotation(Annotation::Create(roi));
            if (annotation.get() != NULL)
            {
              AddAnnotation(annotation.release());
            }
          }
        }
      }
    }
  }
}
