#ifndef MANAGED_UTIL_HPP
#define MANAGED_UTIL_HPP

#include <vector>
#include <cstring>

#include "json.hpp"

#include "ManagedArray.hpp"
#include "Model.hpp"

using json = nlohmann::json;

class ManagedUtil
{
public:

	static std::vector<std::vector<double>> Convert2D(ManagedArray array)
	{
		std::vector<std::vector<double>> model;

		if (array.Length() > 0)
		{
			for (auto y = 0; y < array.y; y++)
			{
				model.push_back(std::vector<double>());

				for (auto x = 0; x < array.x; x++)
					model[y].push_back(array(x, y));
			}
		}

		return model;
	}

	static std::vector<double> Convert1D(ManagedArray array)
	{
		std::vector<double> model;

		for (auto i = 0; i < array.Length(); i++)
		{
			model.push_back(array(i));
		}

		return model;
	}

	static std::vector<std::vector<std::vector<std::vector<double>>>> Convert4DIJ(ManagedArray array)
	{
		std::vector<std::vector<std::vector<std::vector<double>>>> model;

		for (auto i = 0; i < array.i; i++)
		{
			model.push_back(std::vector<std::vector<std::vector<double>>>());

			for (auto j = 0; j < array.j; j++)
			{
				model[i].push_back(std::vector<std::vector<double>>());

				auto temp = ManagedArray(array.x, array.y);

				ManagedOps::Copy4DIJ2D(temp, array, i, j);

				for (auto y = 0; y < temp.y; y++)
				{
					model[i][j].push_back(std::vector<double>());

					for (auto x = 0; x < temp.x; x++)
					{
						model[i][j][y].push_back(temp(x, y));
					}
				}

				ManagedOps::Free(temp);
			}
		}

		return model;
	}

	static ManagedArray Parse1D(json j, std::string field)
	{
		auto model = ManagedArray((int)j[field].size());

		for (int x = 0; x < (int)j[field].size(); x++)
		{
			model(x) = j[field][x];
		}

		return model;
	}

	static ManagedArray Parse2D(json j, std::string field)
	{
		auto model = ManagedArray((int)j[field][0].size(), (int)j[field].size());

		for (int y = 0; y < (int)j[field].size(); y++)
		{
			for (int x = 0; x < (int)j[field][0].size(); x++)
			{
				model(x, y) = j[field][y][x];
			}
		}

		return model;
	}

	static ManagedArray Parse2D(json j, std::string field, int index)
	{
		auto model = ManagedArray((int)j[field][index][0].size(), (int)j[field][index].size());

		for (int y = 0; y < (int)j[field][index].size(); y++)
		{
			for (int x = 0; x < (int)j[field][index][0].size(); x++)
			{
				model(x, y) = j[field][index][y][x];
			}
		}

		return model;
	}

	static ManagedArray Parse4DIJ(json json_string, std::string field)
	{
		auto ii = (int)json_string[field].size();
		auto jj = (int)json_string[field][0].size();
		auto yy = (int)json_string[field][0][0].size();
		auto xx = (int)json_string[field][0][0][0].size();

		auto model = ManagedArray(xx, yy, 1, ii, jj);

		auto temp = ManagedArray(xx, yy);

		for (int i = 0; i < ii; i++)
		{
			for (int j = 0; j < jj; j++)
			{
				for (auto y = 0; y < yy; y++)
				{
					for (auto x = 0; x < xx; x++)
					{
						temp(x, y) = json_string[field][i][j][y][x];
					}
				}

				ManagedOps::Copy2D4DIJ(model, temp, i, j);
			}
		}

		ManagedOps::Free(temp);

		return model;
	}

	static std::vector<double> Vector1D(json j, std::string field, int index)
	{
		auto model = std::vector<double>();

		for (int x = 0; x < (int)j[field][index].size(); x++)
		{
			model.push_back(j[field][index][x]);
		}

		return model;
	}

	static std::string Serialize(std::vector<Model> models)
	{
		json j;

		for (auto i = 0; i < models.size(); i++)
		{
			auto model = models[i];

			json m;

			m["ModelX"] = json(Convert2D(model.ModelX));
			m["ModelY"] = json(Convert1D(model.ModelY));
			m["Type"] = (int)model.Type;
			m["KernelParam"] = json(Convert1D(model.KernelParam));
			m["Alpha"] = json(Convert1D(model.Alpha));
			m["W"] = json(Convert1D(model.W));
			m["B"] = model.B;
			m["C"] = model.C;
			m["Tolerance"] = model.Tolerance;
			m["Category"] = model.Category;
			m["Passes"] = model.Passes;
			m["Iterations"] = model.Iterations;
			m["MaxIterations"] = model.MaxIterations;
			m["Trained"] = model.Trained;

			j["Models"] += m;
		}

		if (models[0].Min.size() > 0 && models[0].Max.size() > 0)
		{
			j["Normalization"] += json(models[0].Min);
			j["Normalization"] += json(models[0].Max);
		}

		return j.dump();
	}

	static std::string Serialize(Model model)
	{
		json j;
		json m;

		m["ModelX"] = json(Convert2D(model.ModelX));
		m["ModelY"] = json(Convert1D(model.ModelY));
		m["Type"] = (int)model.Type;
		m["KernelParam"] = json(Convert1D(model.KernelParam));
		m["Alpha"] = json(Convert1D(model.Alpha));
		m["W"] = json(Convert1D(model.W));
		m["B"] = model.B;
		m["C"] = model.C;
		m["Tolerance"] = model.Tolerance;
		m["Category"] = model.Category;
		m["Passes"] = model.Passes;
		m["Iterations"] = model.Iterations;
		m["MaxIterations"] = model.MaxIterations;
		m["Trained"] = model.Trained;

		j["Models"] += m;

		if (model.Min.size() > 0 && model.Max.size() > 0)
		{
			j["Normalization"] += json(model.Min);
			j["Normalization"] += json(model.Max);
		}

		return j.dump();
	}

	static std::vector<Model> Deserialize(std::string file_name)
	{
		auto models = std::vector<Model>();

		std::ifstream ifs(file_name);

		if (ifs.good())
		{
			json j = json::parse(ifs);

			ifs.close();

			if (j["Models"].size() > 0)
			{
				for (auto i = 0; i < (int)j["Models"].size(); i++)
				{
					auto m = j["Models"][i];

					auto x = Parse2D(m, "ModelX");
					auto y = Parse1D(m, "ModelY");
					auto type = static_cast<KernelType>((int)m["Type"]);
					auto kernelParam = Parse1D(m, "KernelParam");
					auto alpha = Parse1D(m, "Alpha");
					auto w = Parse1D(m, "W");
					auto b = (double)m["B"];
					auto c = (double)m["C"];
					auto tolerance = (double)m["Tolerance"];
					auto category = (int)m["Category"];
					auto passes = (int)m["Passes"];

					y.Reshape(1, y.Length());
					w.Reshape(1, w.Length());
					alpha.Reshape(1, alpha.Length());

					auto model = Model(x, y, type, kernelParam, alpha, w, b, c, tolerance, category, passes);

					model.Min = Vector1D(j, "Normalization", 0);
					model.Max = Vector1D(j, "Normalization", 1);

					models.push_back(model);
				}
			}
		}

		return models;
	}
};
#endif
