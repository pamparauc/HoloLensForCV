//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

namespace Rendering
{
    class SlateRenderer
    {
    public:
        SlateRenderer(
            _In_ const Graphics::DeviceResourcesPtr& deviceResources);

        void CreateDeviceDependentResources();

        void ReleaseDeviceDependentResources();

        void Update(Windows::UI::Input::Spatial::SpatialPointerPose^ pointerPose,
			Windows::Foundation::Numerics::float3 const& offset,
            _In_ const Graphics::StepTimer& timer);

        void Render(
            _In_ const Texture2DPtr& texture);

        // Repositions the sample hologram.
        void PositionHologram(
            Windows::UI::Input::Spatial::SpatialPointerPose^ pointerPose);

        // Property accessors.
        void SetPosition(
            Windows::Foundation::Numerics::float3 pos)
        {
            _position = pos;
        }

        Windows::Foundation::Numerics::float3 GetPosition()
        {
            return _position;
        }

    private:
        // Cached pointer to device resources.
        Graphics::DeviceResourcesPtr _deviceResources;

        // The material we'll use to render this slate.
        std::unique_ptr<SlateMaterial> _slateMaterial;

        // Direct3D resources for the slate geometry.
        Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> _indexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> _modelConstantBuffer;

        // System resources for the slate geometry.
        SlateModelConstantBuffer _modelConstantBufferData;
        uint32 _indexCount = 0;

        // Variables used with the rendering loop.
        bool _loadingComplete = false;

        Windows::Foundation::Numerics::float3 _position = { 0.f, 0.f, -2.f };
        float _rotationInRadians;

		// This is the rate at which the hologram position is interpolated (LERPed) to the current location.
		const float                                         c_lerpRate = 6.0f;

		Windows::Foundation::Numerics::float3               m_targetPosition = { 0.f, 0.f, -2.f };
		Windows::Foundation::Numerics::float3               m_targetUp = { 0.f, 1.f, 0.f };
		Windows::Foundation::Numerics::float3               m_targetForward = { 0.f, 0.f, -1.f };
		Windows::Foundation::Numerics::float3               m_velocity = { 0.f, 0.f, 0.f };
		Windows::Foundation::Numerics::float3               m_normal = { 0.f, 0.f, 1.f };

		DirectX::XMMATRIX maxe_float4x4_world(  Windows::Foundation::Numerics::float3 const& position, 
												Windows::Foundation::Numerics::float3 const& forward, 
												Windows::Foundation::Numerics::float3 const& up);
    };
}
