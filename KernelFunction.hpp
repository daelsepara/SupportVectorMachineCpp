#ifndef KERNEL_FUNCTION_HPP
#define KERNEL_FUNCTION_HPP

#include <cmath>

#include "KernelTypes.hpp"
#include "ManagedMatrix.hpp"

class KernelFunction
{
public:

	static void Vectorize(ManagedArray& x1, ManagedArray& x2)
	{
		// Reshape into column vectors
		ManagedMatrix::Vector(x1);
		ManagedMatrix::Vector(x2);
	}

	static double Multiply(ManagedArray& x1, ManagedArray& x2)
	{
		Vectorize(x1, x2);

		auto tx = ManagedMatrix::Transpose(x1);
		auto xx = ManagedMatrix::Multiply(tx, x2);

		auto x = xx(0);

		ManagedOps::Free(tx);
		ManagedOps::Free(xx);

		return x;
	}

	static double SquaredDiff(ManagedArray& x1, ManagedArray& x2)
	{
		Vectorize(x1, x2);

		double x = 0;

		for (auto i = 0; i < x1.Length(); i++)
		{
			auto d = x1(i) - x2(i);

			x += d * d;
		}

		return x;
	}

	static double Linear(ManagedArray& x1, ManagedArray& x2, ManagedArray& k)
	{
		auto x = Multiply(x1, x2);

		double m = k.Length() > 0 ? k(0) : 1;
		double b = k.Length() > 1 ? k(1) : 0;

		return x * m + b;
	}

	static double Polynomial(ManagedArray& x1, ManagedArray& x2, ManagedArray& k)
	{
		double b = k.Length() > 0 ? k(0) : 0;
		double a = k.Length() > 1 ? k(1) : 1;

		return std::pow(Multiply(x1, x2) + b, a);
	}

	static double Gaussian(ManagedArray& x1, ManagedArray& x2, ManagedArray& k)
	{
		auto x = SquaredDiff(x1, x2);

		double sigma = k.Length() > 0 ? k(0) : 1;

		double denum = 2 * sigma * sigma;

		return std::abs(denum) > 0 ? std::exp(-x / denum) : 0;
	}

	static double Radial(ManagedArray& x1, ManagedArray& x2, ManagedArray& k)
	{
		double sigma = k.Length() > 0 ? k(0) : 1;

		double denum = 2 * sigma * sigma;

		return std::abs(denum) > 0 ? std::exp(-std::sqrt(SquaredDiff(x1, x2)) / denum) : 0;
	}

	static double Sigmoid(ManagedArray& x1, ManagedArray& x2, ManagedArray& k)
	{
		double m = k.Length() > 0 ? k(0) : 1;
		double b = k.Length() > 1 ? k(1) : 0;

		return std::tanh(m * Multiply(x1, x2) / x1.Length() + b);
	}

	static double Fourier(ManagedArray& x1, ManagedArray& x2, ManagedArray& k)
	{
		Vectorize(x1, x2);

		auto z = ManagedArray(x1);

		double prod = 0;

		double m = k.Length() > 0 ? k(0) : 1;

		for (auto i = 0; i < x1.Length(); i++)
		{
			z(i) = std::sin(m + 0.5) * 2;

			auto d = x1(i) - x2(i);

			z(i) = std::abs(d) > 0 ? std::sin(m + 0.5) * d / std::sin(d * 0.5) : z(i);

			prod = (i == 0) ? z(i) : prod * z(i);
		}

		ManagedOps::Free(z);

		return prod;
	}

	static double Run(KernelType type, ManagedArray& x1, ManagedArray& x2, ManagedArray& k)
	{
		double result = 0;

		if (type == KernelType::LINEAR)
		{
			result = Linear(x1, x2, k);
		}

		if (type == KernelType::GAUSSIAN)
		{
			result = Gaussian(x1, x2, k);
		}

		if (type == KernelType::FOURIER)
		{
			result = Fourier(x1, x2, k);
		}

		if (type == KernelType::SIGMOID)
		{
			result = Sigmoid(x1, x2, k);
		}

		if (type == KernelType::RADIAL)
		{
			result = Radial(x1, x2, k);
		}

		if (type == KernelType::POLYNOMIAL)
		{
			result = Polynomial(x1, x2, k);
		}

		return result;
	}
};
#endif
